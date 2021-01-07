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
    uint64_t absolute_seq = unwrap(curSeqno, synSeqno.value(), checkpoint);
    size_t dataLength = payLoad.str().size() + (header.syn ? 1 : 0) + (header.fin ? 1 : 0);
    if (!ackSeqno.has_value() || absolute_seq <= checkpoint) {
        if (absolute_seq + static_cast<uint64_t>(dataLength) <= checkpoint) {
            return;
        }
        if (!(header.syn && dataLength == 1)) {
            _reassembler.push_substring(dataToPush, absolute_seq ? absolute_seq - 1 : 0, header.fin);
        }
		//update ackSeqno and checkpoint
        ackSeqno = curSeqno + dataLength;
        checkpoint = static_cast<uint64_t>(dataLength) + absolute_seq;
		//check the window ,maybe something could write
        auto itor_head = setWindowNode.begin();
        while (itor_head != setWindowNode.end() && itor_head->firstIndex <= checkpoint) {
            if (itor_head->lastIndex >= checkpoint) {
                _reassembler.push_substring(itor_head->data, itor_head->firstIndex - 1, itor_head->finFlag);
                size_t notcoveredLength = itor_head->lastIndex - checkpoint + 1 + (itor_head->finFlag ? 1 : 0);
                ackSeqno = ackSeqno.value() + notcoveredLength;
                checkpoint += notcoveredLength;			
			}
            setWindowNode.erase(itor_head++);		
		}
    } else {  // there is a gap between checkpoint and absolute_seq ,store in the window temporarily
        windowNode curNode{ absolute_seq, absolute_seq + static_cast<uint64_t>(dataToPush.size()) - 1, dataToPush, header.fin };
        auto itor = setWindowNode.lower_bound(curNode);
		//merge pre if exists
        if (itor != setWindowNode.begin()) {
            auto preitor = itor;
            advance(preitor, -1);
            if (preitor->lastIndex >= curNode.firstIndex) {
                if (curNode.lastIndex <= preitor->lastIndex) {
                    curNode= *preitor;
				} else {
                    size_t overlapWindow = static_cast<size_t>(preitor->lastIndex - curNode.firstIndex + 1);
                    curNode.firstIndex = preitor->firstIndex;
                    curNode.data = preitor->data + curNode.data.substr(overlapWindow);
				}
                windowUsed -= preitor->data.size();
                setWindowNode.erase(preitor);		
			}
		}
		//merge next if exists
        while(itor != setWindowNode.end()) {
            if (curNode.lastIndex >= itor->firstIndex) {
                if (curNode.lastIndex < itor->lastIndex) {
                    size_t overlapWindow = static_cast<size_t>(curNode.lastIndex - itor->firstIndex + 1);
                    curNode.lastIndex = itor->lastIndex;
                    curNode.data += itor->data.substr(overlapWindow);
                    curNode.finFlag = itor->finFlag;
				}
                windowUsed -= itor->data.size();
                setWindowNode.erase(itor++);	
			}
		}
        // window may not be enough,if so, erase from big seqno,reserve the small seqno
        setWindowNode.insert(curNode);
        windowUsed += curNode.data.size();
        if (windowUsed > window_size()) {
            size_t excessLength = windowUsed - window_remains();
            auto itor_tail = setWindowNode.rbegin();
            while (itor_tail != setWindowNode.end() && excessLength > 0) {
                size_t datasize = itor_tail->data.size();
                if (datasize <= excessLength) {
                    setWindowNode.erase(itor_tail++);
                    excessLength -= datasize;
                } else {
                    size_t remains = datasize - excessLength;
                    itor_tail->lastIndex = itor_tail->firstIndex + remains - 1;
                    itor_tail->data = itor_tail->data.substr(0, remains);
                    itor_tail->finFlag = false;
                    excessLength = 0;
				}			
			}
		}
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const { return ackSeqno; }

size_t TCPReceiver::window_size() const {
    return _capacity - unassembled_bytes() - _reassembler.stream_out().buffer_size();
}
size_t TCPReceiver::window_remains() const { return window_size() - windowUsed; }
