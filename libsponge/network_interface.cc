#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

EthernetFrame NetworkInterface::make_ethernet_frame_IPV4(const InternetDatagram &dgram,
                                                         EthernetAddress &target_ethaddress) {
    EthernetFrame frame_to_send;
    frame_to_send.header().type = EthernetHeader::TYPE_IPv4;
    frame_to_send.payload().append(dgram.serialize());
    frame_to_send.header().src = _ethernet_address;
    frame_to_send.header().dst = target_ethaddress;
    return frame_to_send;
}
EthernetFrame NetworkInterface::make_ethernet_frame_ARP(const uint32_t &target_ip,
                                                        const EthernetAddress &target_eth,
                                                        uint16_t opcode) {
    EthernetFrame frame_to_send;
    frame_to_send.header().type = EthernetHeader::TYPE_ARP;
    frame_to_send.header().src = _ethernet_address;
    frame_to_send.header().dst = target_eth;
    ARPMessage arp_to_send;
    arp_to_send.sender_ethernet_address = _ethernet_address;
    arp_to_send.sender_ip_address = _ip_address.ipv4_numeric();
	if (!is_same_ethernet(target_eth, ETHERNET_BROADCAST)) {
        arp_to_send.target_ethernet_address = target_eth;
	}
    arp_to_send.target_ip_address = target_ip;
    arp_to_send.opcode = opcode;
    frame_to_send.payload().append(arp_to_send.serialize());
    return frame_to_send;
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    // DUMMY_CODE(dgram, next_hop, next_hop_ip);
    if (ip_to_ethernet.find(next_hop_ip) != ip_to_ethernet.end()) {
        _frames_out.push(make_ethernet_frame_IPV4(dgram, ip_to_ethernet[next_hop_ip]));
    } else {
        temp_store_data[next_hop_ip].push_back(dgram);
        if (!five_seconds_wait.has_value() || five_seconds_wait.value() > 5000) {
            five_seconds_wait = 0;
            _frames_out.push(make_ethernet_frame_ARP(next_hop_ip, ETHERNET_BROADCAST, ARPMessage::OPCODE_REQUEST));
        }
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    // DUMMY_CODE(frame);
    const EthernetAddress &dst = frame.header().dst;
    if (!is_same_ethernet(dst, _ethernet_address) && !is_same_ethernet(dst, ETHERNET_BROADCAST)) {
        return {};
    }
    if (frame.header().type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram parsed_datagram;
        if (parsed_datagram.parse(frame.payload()) == ParseResult::NoError) {
            return parsed_datagram;
        }
    } else if (frame.header().type == EthernetHeader::TYPE_ARP) {
        ARPMessage parsed_arp;
        if (parsed_arp.parse(frame.payload()) == ParseResult::NoError) {
			if (parsed_arp.target_ip_address != _ip_address.ipv4_numeric()) {
                return {};				
			}
            uint32_t remote_ip = parsed_arp.sender_ip_address;
            EthernetAddress remote_ethernet = frame.header().src;
            ip_to_ethernet[remote_ip] = remote_ethernet;
            ip_to_time[remote_ip] = 0;
            switch (parsed_arp.opcode) {
                case ARPMessage::OPCODE_REQUEST:
                    _frames_out.push(make_ethernet_frame_ARP(remote_ip, remote_ethernet, ARPMessage::OPCODE_REPLY));
                    break;
                case ARPMessage::OPCODE_REPLY:
                    if (temp_store_data.find(remote_ip) != temp_store_data.end()) {
                        vector<InternetDatagram> &data_in_queue = temp_store_data[remote_ip];
                        for (size_t i = 0; i < data_in_queue.size(); i++) {
                            _frames_out.push(make_ethernet_frame_IPV4(data_in_queue[i], remote_ethernet));
                        }
                        temp_store_data.erase(remote_ip);
                    }
                    break;
            }
        }
    }
    return {};
}
//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    // DUMMY_CODE(ms_since_last_tick);
    auto itor = ip_to_time.begin();
    while (itor != ip_to_time.end()) {
        uint32_t ip = itor->first;
        size_t cur_time = itor->second;
        if (cur_time + ms_since_last_tick > 30000) {
            ip_to_time.erase(itor++);
            ip_to_ethernet.erase(ip);
        } else {
            itor->second += ms_since_last_tick;
            itor++;
        }
    }
    if (five_seconds_wait.has_value()) {
        five_seconds_wait = five_seconds_wait.value() + ms_since_last_tick;
    }
}

bool NetworkInterface::is_same_ethernet(const EthernetAddress &eth1, const EthernetAddress &eth2) { 
	for (int i = 0; i < 6;i++) {
        if (eth1[i] != eth2[i]) {
            return false;
		}		
	}
    return true;
}