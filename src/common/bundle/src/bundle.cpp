#include "bundle/bundle.hpp"

#include "io/io.h"
#include "core/compiler.h"
#include "std/str.h"

using namespace sm;
using namespace sm::bundle;

sm::Vector<uint8_t> AssetBundle::load_shader(const char *path) const {
    auto& pool = sm::get_pool(sm::Pool::eAssets);
    char *file = str_format(&pool, "%s" CT_NATIVE_PATH_SEPARATOR "%s", m_root, path);
    io_t *io = io_file(file, eAccessRead, &pool);
    io_error_t err = io_error(io);
    CTASSERTF(err == 0, "failed to open shader file %s: %s", file, os_error_string(err, &pool));

    size_t size = io_size(io);
    const void *data = io_map(io, eProtectRead);

    CTASSERTF(size != SIZE_MAX, "failed to get size of shader file %s", file);
    CTASSERTF(data != nullptr, "failed to map shader file %s", file);

    sm::Vector<uint8_t> result(size);
    memcpy(result.data(), data, size);

    io_close(io);

    return result;
}
