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

size_t TCPConnection::time_since_last_segment_received() const { 
	if (timer_for_linger.has_value()) {
        return timer_for_linger.value();
	}
	return 0; 
}
void TCPConnection::send_ACK_segment(WrappingInt32 ackno, uint16_t win) {
    TCPSegment ack_win_segment;
    ack_win_segment.header().seqno = _sender.next_seqno();
    ack_win_segment.header().ack = true;
    ack_win_segment.header().ackno = ackno;
    ack_win_segment.header().win = win;
    _segments_out.push(ack_win_segment);
}
void TCPConnection::check_linger_after_send_and_recv(){
	bool receive_finish = _receiver.stream_out().eof();
        bool send_finish_and_acked = _sender.stream_in().eof() &&
                                     _sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2 &&
                                     _sender.bytes_in_flight() == 0;
        two_way_finish = receive_finish && send_finish_and_acked;
        if (receive_finish && !send_finish_and_acked) {
            _linger_after_streams_finish = false;
        }
	if (!_linger_after_streams_finish && send_finish_and_acked) {
            activeFlag = false;
        }
}

void TCPConnection::segment_received(const TCPSegment &seg) {
    // DUMMY_CODE(seg);
    //!RST
    if (seg.header().rst) {
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        activeFlag = false;
        return;
    }
    //!RECV SEG
    _receiver.segment_received(seg);
    if(seg.header().syn){
	    activeFlag = true;
    }
    check_linger_after_send_and_recv();
    //!ACK
    if (seg.header().ack) {
        // tell sender win and ack
        _sender.ack_received(seg.header().ackno, seg.header().win);       
        // check active
        if (_sender.next_seqno_absolute() > _sender.bytes_in_flight()) {
            activeFlag = true;
        }  
        if (_linger_after_streams_finish && two_way_finish) {
            if (!timer_for_linger.has_value()) {
                timer_for_linger = 0;
            }
        }
    }
    //!send segment only with ack , win,this doesn't occupy sequence number
    if (seg.length_in_sequence_space()) {
        uint16_t win{0};
        if (_receiver.window_size() > numeric_limits<uint16_t>::max()) {
            win = numeric_limits<uint16_t>::max();
        } else {
            win = static_cast<uint16_t>(_receiver.window_size());
        }
        send_ACK_segment(_receiver.ackno().value(), win);
    }
}

bool TCPConnection::active() const { return activeFlag; }

size_t TCPConnection::write(const string &data) {
    // DUMMY_CODE(data);
    size_t bytes_write = _sender.stream_in().write(data);
    _sender.fill_window();
    push_from_Sender_to_connection();
    check_linger_after_send_and_recv();
    return bytes_write;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    //DUMMY_CODE(ms_since_last_tick); 
	//tell sender
    _sender.tick(ms_since_last_tick);
    // send RST
    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        TCPSegment RST_segment;
        RST_segment.header().seqno = _sender.next_seqno();
        RST_segment.header().rst = true;
        _segments_out.push(RST_segment);
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        activeFlag = false;
    }
    // end connection cleanly if linger
    if (two_way_finish && _linger_after_streams_finish) {
        timer_for_linger = timer_for_linger.value() + ms_since_last_tick;
        if (timer_for_linger >= 10 * _cfg.rt_timeout) {
            activeFlag = false;
        }
    }
}

void TCPConnection::end_input_stream() { 
	_sender.stream_in().end_input(); 
	_sender.fill_window();
    push_from_Sender_to_connection();
}

void TCPConnection::connect() {
    if (_sender.next_seqno_absolute() == 0) {
        _sender.fill_window();
        push_from_Sender_to_connection();
	activeFlag = true;
    }
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            // Your code here: need to send a RST segment to the peer
            TCPSegment RST_segment;
            RST_segment.header().seqno = _sender.next_seqno();
            RST_segment.header().rst = true;
            _segments_out.push(RST_segment);
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

void TCPConnection::push_from_Sender_to_connection() { 
    queue<TCPSegment> &sender_queue = _sender.segments_out();
    while (!sender_queue.empty()) {
	TCPSegment& sender_segment = sender_queue.front() ;
	if(_receiver.ackno().has_value()){
		uint16_t win{0};
        	if (_receiver.window_size() > numeric_limits<uint16_t>::max()) {
            		win = numeric_limits<uint16_t>::max();
        	} else {
            		win = static_cast<uint16_t>(_receiver.window_size());
        	}
		sender_segment.header().ack = true;
    		sender_segment.header().ackno = ackno;
   		sender_segment.header().win = win;
	}
        _segments_out.push(sender_segment);
        sender_queue.pop();
    }
}
