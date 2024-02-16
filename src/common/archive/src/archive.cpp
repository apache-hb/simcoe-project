#include "archive/archive.hpp"

using namespace sm;

void Archive::write_bytes(const void *data, size_t size) {
    io_write(mStream.get(), data, size);
}

size_t Archive::read_bytes(void *data, size_t size) {
    return io_read(mStream.get(), data, size);
}

void Archive::write_string(StringView str) {
    uint32_t length = static_cast<uint32_t>(str.size());
    write(length);
    write_bytes(str.data(), str.size());
}

bool Archive::read_string(String& str) {
    uint32_t length;
    if (!read(length))
        return false;

    str.resize(length);
    return read_bytes(str.data(), length) == length;
}
