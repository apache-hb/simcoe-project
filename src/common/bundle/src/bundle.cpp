#include "bundle/bundle.hpp"

#include "io/io.h"

#include "fs/fs.h"

#include "stb/stb_image.h"

using namespace sm;
using namespace sm::bundle;

static BinaryData read_file(fs_t *fs, const char *path) {
    // TODO: fs_open shouldnt return null when no other io_xxx functions do
    io_t *io = fs_open(fs, path, eAccessRead);
    CTASSERTF(io != nullptr, "failed to open file %s", path);

    io_error_t err = io_error(io);
    CTASSERTF(err == 0, "failed to open file %s: %s", path, os_error_string(err, &sm::get_pool(sm::Pool::eAssets)));

    size_t size = io_size(io);
    const void *data = io_map(io, eProtectRead);

    CTASSERTF(size != SIZE_MAX, "failed to get size of file %s", path);
    CTASSERTF(data != nullptr, "failed to map file %s", path);

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

    auto result = read_file(m_vfs, path);

    auto [it, ok] = m_shaders.emplace(path, std::move(result));
    CTASSERTF(ok, "failed to insert shader %s into cache", path);

    m_log.info("loaded shader bytecode {} into cache", path);

    return it->second;
}

Font& AssetBundle::load_font(const char *path) {
    if (auto it = m_fonts.find(path); it != m_fonts.end()) {
        return it->second;
    }

    auto result = read_file(m_vfs, path);

    auto [it, ok] = m_fonts.emplace(path, Font(m_log, result, path));
    CTASSERTF(ok, "failed to insert font %s into cache", path);

    m_log.info("loaded font {} into cache", path);

    return it->second;
}

const Image& AssetBundle::load_image(const char *path, math::uint2 size) {
    if (auto it = m_images.find(path); it != m_images.end()) {
        return it->second;
    }

    auto result = read_file(m_vfs, path);

    int width, height;
    stbi_uc *data = stbi_load_from_memory(result.data(), int(result.size()), &width, &height, nullptr, 4);
    CTASSERTF(data != nullptr, "failed to load image %s", path);

    math::uint2 actual = {unsigned(width), unsigned(height)};

    sm::UniqueArray<uint8_t> pixels{data, size_t(width * height) * 4};
    Image image{actual, DataFormat::eRGBA8_UINT, std::move(pixels)};

    auto [it, ok] = m_images.emplace(path, std::move(image));
    CTASSERTF(ok, "failed to insert image %s into cache", path);

    m_log.info("loaded image {} into cache", path);

    return it->second;
}

void AssetBundle::open_fs() {
    IArena& pool = sm::get_pool(sm::Pool::eAssets);
    m_vfs = fs_physical(m_path, &pool);
    m_log.info("opened vfs for {}", m_path);
}
