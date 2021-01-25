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

EthernetFrame NetworkInterface::make_ethernet_frame_IPV4(InternetDatagram &dgram, EthernetAddress &ethaddress) {
    EthernetFrame frame_to_send;
    frame_to_send.header().type = EthernetHeader::TYPE_IPv4;
    frame_to_send.payload().append(dgram.serialize());
    frame_to_send.header().src = _ethernet_address;
    frame_to_send.header().dst = ethaddress;
    return frame_to_send;
}
EthernetFrame NetworkInterface::make_ethernet_frame_ARP(uint32_t &ipaddress) {
    EthernetFrame frame_to_send;
    frame_to_send.header().type = EthernetHeader::TYPE_ARP;
    frame_to_send.header().src = _ethernet_address;
    frame_to_send.header().dst = ETHERNET_BROADCAST;
    ARPMessage arp_to_send;
    arp_to_send.sender_ethernet_address = _ethernet_address;
    arp_to_send.sender_ip_address = _ip_address;
    arp_to_send.target_ethernet_address = ETHERNET_BROADCAST;
    arp_to_send.target_ip_address = ipaddress;
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
        if (!five_seconds_wait.has_value() || five_seconds_wait.has_value() && five_seconds_wait.value() > 5000) {
            five_seconds_wait = 0;
            _frames_out.push(make_ethernet_frame_ARP(next_hop_ip));
        }
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    // DUMMY_CODE(frame);
    // ARP
    if (frame.header().type == TYPE_IPv4) {
        frame.payload().buffers()



    } else if (frame.header().type == TYPE_ARP) {
        //check opcode



    }
    return {};
    //************************************************************************************************************
    if (frame.header().dst == ETHERNET_BROADCAST) {
        ARPMessage arp_recv, arp_send;
        if (arp_recv.parse(frame.payload().buffers()[0]) == ParseResult::NoError && arp_recv.supported() &&
            arp_recv.opcode == OPCODE_REQUEST && arp_recv.target_ip_address == _ip_address.ipv4_numeric()) {
            uint32_t remote_ip = arp_recv.sender_ip_address;
            EthernetAddress remote_ethernet = frame.header().src;
            // store ip ethernet info
            ip_to_ethernet[remote_ip] = remote_ethernet;
            ip_to_time[remote_ip] = 0;
            // set src dst info
            arp_send.sender_ethernet_address = _ethernet_address;
            arp_send.sender_ip_address = _ip_address.ipv4_numeric();
            arp_send.target_ethernet_address = remote_ethernet;
            arp_send.target_ip_address = remote_ip;
            // set reply code
            arp_send.opcode = OPCODE_REPLY;
            // send arp reply
            InternetDatagram internetData_send;
            string arp_send_serialize = arp_send.serialize();
            internetData_send.payload().append(arp_send_serialize);
            internetData_send.header().src = _ip_address.ipv4_numeric();
            internetData_send.header().dst = remote_ip;
            return internetData_send;
        } else {
            return {};
        }
    }
    // IPV4
    if (frame.header().dst == _ethernet_address) {
        ARPMessage arp_reply;
        if (arp_reply.parse(frame.payload().buffers()[0]) == ParseResult::NoError && arp_reply.supported() &&
            arp_reply.opcode == OPCODE_REPLY && arp_reply.target_ip_address == _ip_address.ipv4_numeric()) {
            uint32_t remote_ip = arp_reply.sender_ip_address;
            EthernetAddress remote_ethernet = frame.header().src;
            ip_to_ethernet[remote_ip] = remote_ethernet;
            ip_to_time[remote_ip] = 0;
            if (temp_store_data.find(remote_ip) != temp_store_data.end()) {
                for (int i = 0; i < temp_store_data[remote_ip].size(); i++) {
                    InternetDatagram &data_in_queue = temp_store_data[remote_ip][i];
                    _frames_out.push(make_ethernet_frame_IPV4(data_in_queue, remote_ethernet));
                }
                temp_store_data.erase(remote_ip);
            }
        }
        InternetDatagram internetData_recv;
        if (internetData_recv.parse(frame.payload().buffers()[0]) == ParseResult::NoError) {
            return internetData_recv;
        }
    }

    return {};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    // DUMMY_CODE(ms_since_last_tick);
    auto itor = ip_to_time.first();
    while (itor != ip_to_time.end()) {
        uint32_t ip = itor->first;
        size_t cur_time = itor->second;
        if (cur_time + ms_since_last_tick > 30000) {
            ip_to_time.erase(itor++);
            ip_to_ethernet.erase(ip);
        } else {
            itor->second += ms_since_last_tick;
        }
    }
    if (five_seconds_wait.has_value()) {
        five_seconds_wait = five_seconds_wait.value() + ms_since_last_tick;
    }
}
