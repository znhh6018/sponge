#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity)
    : bufferCapacity(capacity), readCount(0), writeCount(0), inputEnded(false), _error(false) {
    // DUMMY_CODE(capacity);
}

size_t ByteStream::write(const string &data) {
    // DUMMY_CODE(data);
    size_t enabledWrite = min(data.size(), remaining_capacity());
    for (size_t i = 0; i < enabledWrite; i++) {
        streamBuffer.push_back(data[i]);
    }
    writeCount += enabledWrite;
    return enabledWrite;
}

//! \param[in] len bytes will be copied from the output side of the buffer
std::string ByteStream::peek_output(const size_t len) const {
    // DUMMY_CODE(len);
    size_t enabledPeek = min(len, streamBuffer.size());

    std::string res;
    res.assign(streamBuffer.begin(), streamBuffer.begin() + enabledPeek);
    return res;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    // DUMMY_CODE(len);
    size_t enabledRead = min(len, streamBuffer.size());
    for (size_t i = 0; i < enabledRead; i++) {
        streamBuffer.pop_front();
    }
    readCount += enabledRead;  // not in descriptions,waste a lot of time
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    // DUMMY_CODE(len);
    size_t enabledRead = min(len, streamBuffer.size());
    std::string res;
    res.assign(streamBuffer.begin(), streamBuffer.begin() + enabledRead);
    for (size_t i = 0; i < enabledRead; i++) {
        streamBuffer.pop_front();
    }
    readCount += enabledRead;
    return res;
}

void ByteStream::end_input() { inputEnded = true; }

bool ByteStream::input_ended() const { return inputEnded; }

size_t ByteStream::buffer_size() const { return streamBuffer.size(); }

bool ByteStream::buffer_empty() const { return streamBuffer.empty(); }

bool ByteStream::eof() const { return inputEnded && streamBuffer.empty(); }

size_t ByteStream::bytes_written() const { return writeCount; }

size_t ByteStream::bytes_read() const { return readCount; }

size_t ByteStream::remaining_capacity() const { return bufferCapacity - streamBuffer.size(); }
