#include "archive/io.hpp"

using namespace sm;

Io Io::file(const char *path, archive::IoAccess access) {
    auto& pool = get_pool(Pool::eAssets);
    return Io(io_file(path, access.as_facade(), &pool));
}

IoError Io::error() const {
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
