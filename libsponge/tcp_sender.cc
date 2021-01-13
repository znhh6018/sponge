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

uint64_t TCPSender::bytes_in_flight() const { 
	return  _next_seqno - accepted_ack_absolute_seq;
}

void TCPSender::sendSegment(TCPSegment &seg) {
    size_t sequence_space = seg.length_in_sequence_space();
    if (sequence_space == 0) {
        return;
    }
    seg.header().seqno = next_seqno();
    _next_seqno += sequence_space;
    _segments_out.push(seg);
    _segments_out_not_ack.push(seg);
    if (!timeElapsed.has_value()) {
        timeElapsed = 0;
    }
}

void TCPSender::fill_window() {
    if (finFlag ) {
        return;
	}
    TCPSegment seg_to_send;
    if (_next_seqno == 0) {
        seg_to_send.header().syn = true;
        sendSegment(seg_to_send);
        return;
    } 
	if ( windowSize <= bytes_in_flight()) {
        return;
	}
    size_t windowSize_without_flyingbytes = windowSize - static_cast<size_t>(bytes_in_flight());
    while (windowSize_without_flyingbytes > 0 && !stream_in().eof()) {
        ByteStream &byteSource = stream_in();
        size_t bytesCount = min(TCPConfig::MAX_PAYLOAD_SIZE, windowSize_without_flyingbytes);
        string payLoads = byteSource.read(bytesCount);
        size_t payLoads_size = payLoads.size();
        if ( payLoads_size == 0) {
            break;
		}
        windowSize_without_flyingbytes -= payLoads_size;      // space remains
        seg_to_send.payload() = Buffer(std::move(payLoads));  // assignment operator buffer.cc
        if (byteSource.eof() && windowSize_without_flyingbytes) {
            seg_to_send.header().fin = true;
            finFlag = true;
        }
        sendSegment(seg_to_send);
    }
    if (windowSize_without_flyingbytes > 0 && stream_in().eof() && !finFlag) {  //!finflag  means ,finflag has not been writed 
        seg_to_send.header().fin = true;
        finFlag = true;
        sendSegment(seg_to_send);
    }	
}
//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t ack_received_absolute_seq = unwrap(ackno, _isn, accepted_ack_absolute_seq);
    if (ack_received_absolute_seq < accepted_ack_absolute_seq || ack_received_absolute_seq > _next_seqno) {
        return;
	}
    if (window_size == 0) {
        windowSize_zero_flag = true;
        windowSize = 1;
    } else {
        windowSize_zero_flag = false;
        windowSize = window_size;
	}
    accepted_ack_absolute_seq = ack_received_absolute_seq;
    while (!_segments_out_not_ack.empty()) {
        TCPSegment& seg = _segments_out_not_ack.front();
        uint64_t absoluteSeq_notAckInqueue = unwrap(seg.header().seqno, _isn, _next_seqno);
        size_t segLen = seg.length_in_sequence_space();
        if (absoluteSeq_notAckInqueue + static_cast<uint64_t>(segLen) <= accepted_ack_absolute_seq) {
            _segments_out_not_ack.pop();
            consecutive_retransmissions_count = 0;
            cur_retransmission_timeout = _initial_retransmission_timeout;
            if (_segments_out_not_ack.empty()) {
                timeElapsed.reset();				
			} else {
                timeElapsed = 0;
			}
        } else{
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
    timeElapsed = timeElapsed.value() + ms_since_last_tick;
    if (timeElapsed < cur_retransmission_timeout) {
        return;
	}
    if (!windowSize_zero_flag) {
        ++consecutive_retransmissions_count;
        cur_retransmission_timeout *= 2;
	}
    TCPSegment segToRetrans = _segments_out_not_ack.front();
    _segments_out.push(segToRetrans);
    timeElapsed = 0;
}

unsigned int TCPSender::consecutive_retransmissions() const { 
	return consecutive_retransmissions_count; 
}

void TCPSender::send_empty_segment() { 
	TCPSegment empty_segment;
    empty_segment.header().seqno = next_seqno();
    _segments_out.push(empty_segment);
}
