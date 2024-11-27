#include "stdafx.hpp"

#include "archive/io.hpp"

#include "core/memory.h"

using namespace sm;

Io Io::file(const char *path, os_access_t access) {
    return Io(io_file(path, access, get_default_arena()));
}

OsError Io::error() const {
    return io_error(get());
}

const char *Io::name() const {
    return io_name(get());
}

size_t Io::size() const {
    return io_size(get());
}

size_t Io::read_bytes(void *data, size_t size) {
    return io_read(get(), data, size);
}

size_t Io::write_bytes(const void *data, size_t size) {
    return io_write(get(), data, size);
}
