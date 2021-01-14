#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return {}; }

void TCPConnection::segment_received(const TCPSegment &seg) { 
	//DUMMY_CODE(seg); 
	if (seg.header().rst) {
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        activeFlag = false;
        return;
	}
    _receiver.segment_received(seg);
    if (seg.header().ack) {
        _sender.ack_received(seg.header().ackno,seg.header().win);
	}
	//send segment only with ack , win,this doesn't occupy sequence number
    if (seg.length_in_sequence_space()) {
        TCPSegment ack_win_segment;
        ack_win_segment.header().seqno = _sender.next_seqno();
        ack_win_segment.header().ack = true;
        ack_win_segment.header().ackno = _receiver.ackno();
        ack_win_segment.header().win = _receiver.window_size();
        _segments_out.push(ack_win_segment);
	}
}

bool TCPConnection::active() const { return activeFlag; }

size_t TCPConnection::write(const string &data) {
    //DUMMY_CODE(data);
	ByteStream &_stream = _sender.stream_in();
    size_t bytes_write =  _stream.write(data);
    _sender.fill_window();
    return bytes_write;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
	//DUMMY_CODE(ms_since_last_tick); 

}

void TCPConnection::end_input_stream() { 
	_sender.stream_in().end_input(); 
}


void TCPConnection::connect() { 
	if (_sender.next_seqno_absolute() == 0) {
        _sender.fill_window();
	}
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
