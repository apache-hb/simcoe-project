#include "archive/archive.hpp"

#include "core/units.hpp"

using namespace sm;

void Archive::write_bytes(const void *data, size_t size) {
    io_write(mStream.get(), data, size);
}

void Archive::write_length(size_t length) {
    write(int_cast<uint32_t>(length));
}

void Archive::write_string(StringView str) {
    write_length(str.size());
    write_bytes(str.data(), str.size());
}

size_t Archive::read_bytes(void *data, size_t size) {
    return io_read(mStream.get(), data, size);
}

bool Archive::read_length(size_t& length) {
    uint32_t len;
    if (!read(len))
        return false;

    length = len;
    return true;
}

bool Archive::read_string(String& str) {
    size_t length;
    if (!read_length(length))
        return false;

    str.resize(length);
    return read_bytes(str.data(), length) == length;
}
