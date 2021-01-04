#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    //DUMMY_CODE(seg);
    const TCPHeader& header = seg.header();
    const Buffer& payLoad = seg.payload(); 
    //decide if connection start
    if(header.syn && !synSeqno.has_value()){
        synSeqno = header.seqno;
        ackSeqno = header.seqno;
    }
    if(!synSeqno.has_value()){
        return;    
    }
    //connection start
    size_t dataLength = payLoad.str().size() + header.syn?1:0 + header.fin?1:0;
    
    
    
    //no need to compare with the window size,has been designed in push_string
    WrappingInt32 curSeqno = header.seqno;
    uint64_t absolute_seq = unwrap(curSeqno,synSeqno,);
    string dataToPush = payLoad.copy();
    _reassembler.push_string(dataToPush,absolute_seq,header.fin);
    
    
    
    
    
}

optional<WrappingInt32> TCPReceiver::ackno() const { return {}; }

size_t TCPReceiver::window_size() const { return _capacity - unassembled_bytes() -  _reassembler.stream_out().buffer_size(); }
