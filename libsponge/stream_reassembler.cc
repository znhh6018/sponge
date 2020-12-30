#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity), _capacity(capacity), nextByteIndex(0), unassembled_bytes_count(0), finalByteIndex(0) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const uint64_t index, const bool eof) {
    // DUMMY_CODE(data, index, eof);
    // decide whether this data store or discard
    size_t curDataSize = data.size();
    size_t curLength = _output.buffer_size() + unassembled_bytes();
    if (curDataSize + curLength > _capacity) {
        return;
    }
    //!decide eof
    if (eof) {
        finalByteIndex = static_cast<uint64_t>(index + curDataSize - 1);
    }
    //!push string data into setNode and merge
    //!think of duplicate or overlapping
    dataNode curDataNode{index, index + curDataSize - 1, data};
    auto itor = setNode.lower_bound(curDataNode);
    if (itor == setNode.end()) {
        setNode.insert(curDataNode);
        unassembled_bytes_count += curDataSize;
    } else {
		//! merge pre
		if (itor != setNode.begin()){
            auto preitor = itor;
            --preitor;
            if (curDataNode.firstIndex <= preitor->lastIndex && curDataNode.lastIndex > preitor->lastIndex) {
                mergePreNode(preitor, curDataNode);
                unassembled_bytes_count -= preitor->data.size();
                setNode.erase(preitor);
			}
		}
		//! merge next
        while (itor != setNode.end() && curDataNode.lastIndex >= itor->firstIndex) {
            mergeNextNode(itor, curDataNode);
            unassembled_bytes_count -= itor->data.size();
            setNode.erase(itor++);
		}
        setNode.insert(curDataNode);
        unassembled_bytes_count += curDataNode.data.size();
    }
    //!write to stream_out
    auto begin = setNode.begin();
    while (begin != setNode.end() && begin->firstIndex == nextByteIndex) {
        _output.write(begin->data);
        size_t dataLength = data.size();
        nextByteIndex += static_cast<uint64_t>(dataLength);
        unassembled_bytes_count -= dataLength;
        setNode.erase(begin++);
	}
}
void StreamReassembler::mergePreNode(set<dataNode, cmp>::iterator &preitor, dataNode &DataNode) {
	DataNode.firstIndex = preitor->firstIndex;
    uint64_t overlapLen = preitor->lastIndex - DataNode.firstIndex + 1;
    DataNode.data = preitor->data + DataNode.data.substr(static_cast<size_t>(overlapLen));  // data[overlapLen] -- data[end]
}

void StreamReassembler::mergeNextNode(set<dataNode, cmp>::iterator &nextitor, dataNode &DataNode) {
    DataNode.lastIndex = max(DataNode.lastIndex, nextitor->lastIndex);
    uint64_t overlapLen = preitor->lastIndex - DataNode.firstIndex + 1;
    DataNode.data = DataNode.data + nextitor->data.substr(static_cast<size_t>(overlapLen));  // data[overlapLen] -- data[end]
}

size_t StreamReassembler::unassembled_bytes() const { return unassembled_bytes_count; }

bool StreamReassembler::empty() const { return nextByteIndex == finalByteIndex + 1; }
