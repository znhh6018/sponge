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
    // initialize syn
    if (header.syn && !synSeqno.has_value()) {
        synSeqno = header.seqno;
        ackSeqno = header.seqno + 1;
    }
    if (!synSeqno.has_value()) {
        return;
    }
    // SYN true,push string
    WrappingInt32 curSeqno = header.seqno;
    string dataToPush = payLoad.copy();
    uint64_t absolute_seq = unwrap(curSeqno, synSeqno.value(), checkpoint);
    size_t dataLength = payLoad.str().size() + (header.syn ? 1 : 0) + (header.fin ? 1 : 0);
    if (header.syn && dataLength == 1) {
        return;
    }
    _reassembler.push_substring(dataToPush, absolute_seq ? absolute_seq - 1 : 0, header.fin);
    checkpoint = _reassembler.getnextByteIndex();//next stream index
    ackSeqno = wrap(checkpoint + 1, synSeqno.value());
}

optional<WrappingInt32> TCPReceiver::ackno() const { return ackSeqno; }

size_t TCPReceiver::window_size() const {
    return _capacity - _reassembler.stream_out().buffer_size();
}
