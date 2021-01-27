#include "router.hh"

#include <iostream>

using namespace std;

// Dummy implementation of an IP router

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

// For Lab 6, please replace with a real implementation that passes the
// automated checks run by `make check_lab6`.

// You will need to add private members to the class declaration in `router.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";

    DUMMY_CODE(route_prefix, prefix_length, next_hop, interface_num);
    // Your code here.
    for (int i = 0; i <= 32 - prefix_length; i++) {
        value_to_ip[route_prefix>>i] = route_prefix;
	}
    ip_to_pair[route_prefix] = make_pair(next_hop, interface_num);
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    DUMMY_CODE(dgram);
    // Your code here.
    size_t dst_ip = dgram.header().dst; 
	for (int i = 0; i < 32; i++) {
        size_t vlaue = (dst_ip >> i);
        if (value_to_ip.find(value) != value_to_ip.end()) {
            size_t longest_ip = value_to_ip[value];
            const optional<Address> next_address = ip_to_pair[longest_ip].first;
            const size_t interface_num = ip_to_pair[longest_ip].second;
            if (next_address.has_value()) {
                interface(interface_num).send_datagram(dgram, next_address);
			} else {
                interface(interface_num).send_datagram(dgram, Address::from_ipv4_numeric(dst_ip));
			}
            return;		
		}
	}
    if (dgram.header().ttl == 0) {
        return;
	}
    dgram.header().ttl--;
}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}
