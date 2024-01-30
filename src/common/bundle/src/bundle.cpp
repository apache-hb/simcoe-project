#include "bundle/bundle.hpp"

#include "io/io.h"
#include "core/compiler.h"
#include "std/str.h"

using namespace sm;
using namespace sm::bundle;

static BinaryData read_file(const char *root, const char *path) {
    auto& pool = sm::get_pool(sm::Pool::eAssets);
    char *file = str_format(&pool, "%s" CT_NATIVE_PATH_SEPARATOR "%s", root, path);
    io_t *io = io_file(file, eAccessRead, &pool);
    io_error_t err = io_error(io);
    CTASSERTF(err == 0, "failed to open file %s: %s", file, os_error_string(err, &pool));

    size_t size = io_size(io);
    const void *data = io_map(io, eProtectRead);

    CTASSERTF(size != SIZE_MAX, "failed to get size of file %s", file);
    CTASSERTF(data != nullptr, "failed to map file %s", file);

    BinaryData result(size);
    memcpy(result.data(), data, size);

    io_close(io);

    return result;
}

BinarySpan AssetBundle::load_shader(const char *path) {
    // TODO: timing
    if (auto it = m_shaders.find(path); it != m_shaders.end()) {
        return it->second;
    }

    auto result = read_file(m_root, path);

    auto [it, ok] = m_shaders.emplace(path, std::move(result));
    CTASSERTF(ok, "failed to insert shader %s into cache", path);

    m_log.info("placed shader {} into cache", path);

    return it->second;
}

Font& AssetBundle::load_font(const char *path) {
    if (auto it = m_fonts.find(path); it != m_fonts.end()) {
        return it->second;
    }

    auto result = read_file(m_root, path);

    auto [it, ok] = m_fonts.emplace(path, Font(m_log, result, path));
    CTASSERTF(ok, "failed to insert font %s into cache", path);

    m_log.info("placed font {} into cache", path);

    return it->second;
}
