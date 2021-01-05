#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    // DUMMY_CODE(seg);
    const TCPHeader &header = seg.header();
    const Buffer &payLoad = seg.payload();
    // decide if connection start
    if (header.syn && !synSeqno.has_value()) {
        synSeqno = header.seqno;
    }
    if (!synSeqno.has_value()) {
        return;
    }
    // SYN true,push string
    WrappingInt32 curSeqno = header.seqno;
    string dataToPush = payLoad.copy();
    uint64_t absolute_seq = unwrap(curSeqno, synSeqno, checkpoint);
    if (dataToPush.size() != 0) {
        _reassembler.push_substring(dataToPush, absolute_seq?absolute_seq - 1:0, header.fin);
	}
	//get ackseqno
    if (!ackSeqno.has_value() || ackSeqno == curSeqno) {
        size_t dataLength = payLoad.str().size() + header.syn ? 1 : 0 + header.fin ? 1 : 0;
        ackSeqno = curSeqno +  WrappingInt32(dataLength);
        checkpoint += static_cast<uint64_t>(dataLength);
	}
}

optional<WrappingInt32> TCPReceiver::ackno() const { return ackSeqno; }

size_t TCPReceiver::window_size() const {
    return _capacity - unassembled_bytes() - _reassembler.stream_out().buffer_size();
}
