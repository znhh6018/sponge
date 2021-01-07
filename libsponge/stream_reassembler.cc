#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity)
    , _capacity(capacity)
    , nextByteIndex(0)
    , unassembled_bytes_count(0)
    , finalByteIndex(0)
    , EmptysubstringWithEof_flag(false)
    , iseof (false){}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const uint64_t index, const bool eof) {
    // DUMMY_CODE(data, index, eof);
    // decide whether this data store or discard
    size_t curDataSize = data.size();
    if (eof) {
        iseof = true;
        finalByteIndex = curDataSize == 0 ? index : index + curDataSize - 1;
    }
    if (curDataSize == 0) {
        if (eof) {
            EmptysubstringWithEof_flag = true;
        } else {
            return;
        }
    } else {
         //!push string data into setNode and merge
         //!think of duplicate or overlapping
         dataNode curDataNode{index, index + curDataSize - 1, data};
         if (nextByteIndex > curDataNode.firstIndex && nextByteIndex <= curDataNode.lastIndex) {
             uint64_t writed = nextByteIndex - curDataNode.firstIndex;
             curDataNode.data = curDataNode.data.substr(static_cast<size_t>(writed));
             curDataNode.firstIndex = curDataNode.firstIndex + writed;
	 } else if (nextByteIndex > curDataNode.lastIndex) {
             return;
	 }
         auto itor = setNode.lower_bound(curDataNode);		 
         // merge pre if exists
         if (itor != setNode.begin()) {
             auto preitor = itor;
             advance(preitor, -1);
             if (curDataNode.firstIndex <= preitor->lastIndex) {
                 mergePreNode(preitor, curDataNode);
                 unassembled_bytes_count -= preitor->data.size();
                 setNode.erase(preitor);
             }
         }
         // merge next if exists
         while (itor != setNode.end() && curDataNode.lastIndex >= itor->firstIndex) {
             mergeNextNode(itor, curDataNode);
             unassembled_bytes_count -= itor->data.size();
             setNode.erase(itor++);
         }
	 //discard surplus bytes,discard from the tail
         size_t nodeLengthAfterMerge = curDataNode.data.size();
         size_t totalLength = nodeLengthAfterMerge + unassembled_bytes() + _output.buffer_size();
         setNode.insert(curDataNode);
         unassembled_bytes_count += nodeLengthAfterMerge;
         if (totalLength > _capacity) {
             size_t discardLength = totalLength - _capacity;
             auto tail_itor = setNode.rbegin();
             while (tail_itor != setNode.rend() && discardLength > 0) {
                 size_t datasize = tail_itor->data.size();
                 auto forward_itor = (++tail_itor).base();
                 if (datasize <= discardLength) {                     
                     discardLength -= datasize;
                 } else {
                     dataNode tempNode = *forward_itor;
                     size_t remains = datasize - discardLength;
                     tempNode.lastIndex = tempNode.firstIndex + remains - 1;
                     tempNode.data = tempNode.data.substr(0, remains);
                     discardLength = 0;
                     setNode.insert(tempNode);
				 }
                 setNode.erase(forward_itor);
                 unassembled_bytes_count -= datasize;				
			 }
		}
         
         //!write to stream_out
         auto begin = setNode.begin();
         while (begin != setNode.end() && begin->firstIndex == nextByteIndex) {
             _output.write(begin->data);
             size_t dataLength = begin->data.size();
             nextByteIndex += static_cast<uint64_t>(dataLength);
             unassembled_bytes_count -= dataLength;
             setNode.erase(begin++);
         }
	}  
    // end input
    if (empty()) {
        stream_out().end_input();
    }
}
void StreamReassembler::mergePreNode(set<dataNode, cmp>::iterator &preitor, dataNode &DataNode) {   
    if (DataNode.lastIndex <= preitor->lastIndex) {
        DataNode.lastIndex = preitor->lastIndex;
        DataNode.data = preitor->data;
    } else {
        size_t overlapLen = static_cast<size_t>(preitor->lastIndex - DataNode.firstIndex + 1);
        DataNode.data = preitor->data + DataNode.data.substr(overlapLen);  // data[overlapLen] -- data[end]
    }
    DataNode.firstIndex = preitor->firstIndex;
}

void StreamReassembler::mergeNextNode(set<dataNode, cmp>::iterator &nextitor, dataNode &DataNode) {
    if (DataNode.lastIndex < nextitor->lastIndex) {
        size_t overlapLen = static_cast<size_t>(DataNode.lastIndex - nextitor->firstIndex + 1);
        DataNode.lastIndex = nextitor->lastIndex;       
        DataNode.data = DataNode.data + nextitor->data.substr(overlapLen);  // data[overlapLen] -- data[end]
    }
}

size_t StreamReassembler::unassembled_bytes() const { return unassembled_bytes_count; }

bool StreamReassembler::empty() const { 
	return EmptysubstringWithEof_flag ? nextByteIndex == finalByteIndex
                                      : iseof && nextByteIndex == finalByteIndex + 1;
}
