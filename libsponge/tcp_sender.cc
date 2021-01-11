#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , cur_retransmission_timeout(retx_timeout) {}

uint64_t TCPSender::bytes_in_flight() const { return {}; }

void TCPSender::fill_window() {
    TCPSegment seg_to_send;
    if (_next_seqno == 0) {
        seg_to_send.header().syn = true;
        seg_to_send.header().seqno = _isn;

    } else {
        ByteStream &byteSource = stream_in();
        size_t bytesCount = min(TCPConfig::MAX_PAYLOAD_SIZE, windowSize ? windowSize : 1);
        string payLoads = byteSource.read(bytesCount);
        seg_to_send.payload() = payLoads;  // assignment operator buffer.cc
        seg_to_send.header().seqno = next_seqno();
        if (byteSource.eof()) {
            seg_to_send.header().fin = true;
        }
	}
    _next_seqno += seg_to_send.length_in_sequence_space();
    _segments_out.push(seg_to_send);
    _segments_out_not_ack.push(seg_to_send);
    if ( !timeElapsed.has_value()) {
        timeElapsed = 0;
	}
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // DUMMY_CODE(ackno, window_size);
    windowSize = window_size;
    accepted_ack_absolute_seq = unwrap(ackno, _isn, accepted_ack_absolute_seq);
    while (!_segments_out_not_ack.empty()) {
        TCPSegment& seg = _segments_out_not_ack.front();
        uint64_t absoluteSeq_notAckInqueue = unwrap(seg.header().seqno, _isn, _next_seqno);
        size_t segLen = seg.length_in_sequence_space();
        if (absoluteSeq_notAckInqueue + static_cast<uint64_t>(segLen) <= accepted_ack_absolute_seq) {
            _segments_out_not_ack.pop();
            --consecutive_retransmissions_count;
            cur_retransmission_timeout = _initial_retransmission_timeout;
            if (_segments_out_not_ack.empty()) {
                timeElapsed.reset();				
			} else {
                timeElapsed = 0;
			}
        } else {
            break;
		}
	}
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    // DUMMY_CODE(ms_since_last_tick);
	//no segment in _segments_out_not_ack
    if (!timeElapsed.has_value()) {
        return;
	}
    timeElapsed = timeElapsed + ms_since_last_tick;
    if (timeElapsed < cur_retransmission_timeout) {
        return;
	}
    ++consecutive_retransmissions_count;
    cur_retransmission_timeout *= 2;
    TCPSegment &segToRetrans = _segments_out_not_ack.front();
    _segments_out.push(segToRetrans);
    timeElapsed = 0;
}

unsigned int TCPSender::consecutive_retransmissions() const { 
	return consecutive_retransmissions_count; 
}

void TCPSender::send_empty_segment() { 
	TCPSegment empty_segment;
    empty_segment.header().seqno = next_seqno();
    _segments_out.push(seg_to_send);
}
