#include "bundle/bundle.hpp"

#include "io/io.h"
#include "core/compiler.h"
#include "std/str.h"

#include "unzip.h"
#include "iowin32.h"

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

    sm::Vector<uint8_t> pixels;
    pixels.resize(width * height * 4);
    memcpy(pixels.data(), data, pixels.size());

    Image image{
        math::uint2{static_cast<unsigned int>(width), static_cast<unsigned int>(height)},
        DataFormat::eRGBA8_UINT,
        std::move(pixels)
    };

    stbi_image_free(data);

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

#define SM_ERROR_ZLIB(err, msg) NEVER("| zlib error: %s (%d)\n| expr: " msg, zError(err), err)

#define SM_ASSERT_ZLIB(expr)                                 \
    do {                                                       \
        if (auto result = (expr); result != Z_OK) {            \
            SM_ERROR_ZLIB(result, #expr); \
        }                                                      \
    } while (0)

#define SM_ERROR_UNZ(err, msg) NEVER("| unzip error: %s (%d)\n| expr: " msg, zError(err), err)

#define SM_ASSERT_UNZ(expr)                                 \
    do {                                                       \
        if (auto result = (expr); result != UNZ_OK) {            \
            SM_ERROR_UNZ(result, #expr); \
        }                                                      \
    } while (0)

void AssetBundle::extract_dirent(void *zip, const char *path) {
    IArena& pool = sm::get_pool(sm::Pool::eAssets);

    char filename[256];
    unz_file_info64 info;
    SM_ASSERT_UNZ(unzGetCurrentFileInfo64(zip, &info, filename, sizeof(filename), nullptr, 0, nullptr, 0));

    // is this a directory?
    if (char_is_any_of(filename[info.size_filename - 1], CT_PATH_SEPERATORS)) {
        m_log.info("creating directory {}", filename);
        fs_dir_create(m_vfs, filename);
        return;
    }

    // otherwise read the file

    // there may not be a directory already created
    char *dir = str_directory(filename, &pool);
    m_log.info("extracting {} compressed = {}, uncompressed = {}", filename, info.compressed_size, info.uncompressed_size);
    fs_dir_create(m_vfs, dir);

    SM_ASSERT_UNZ(unzOpenCurrentFile(zip));

    io_t *io = fs_open(m_vfs, filename, eAccessWrite);

    void *buffer = ARENA_MALLOC(info.uncompressed_size, filename, zip, &pool);
    int err = 0;
    do {
        err = unzReadCurrentFile(zip, buffer, info.uncompressed_size);
        if (err < 0) {
            SM_ERROR_UNZ(err, "failed to read file");
        }

        if (err > 0) {
            io_write(io, buffer, err);
        }
    } while (err > 0);

    io_close(io);

    SM_ASSERT_UNZ(unzCloseCurrentFile(zip));

    arena_free(buffer, info.uncompressed_size, &pool);
}

void AssetBundle::inflate_zip(const char *path) {
    IArena& pool = sm::get_pool(sm::Pool::eAssets);
    m_vfs = fs_virtual("bundle", &pool);

    m_log.info("inflating zip file {}", path);

    zlib_filefunc64_def ffunc{};
    fill_win32_filefunc64A(&ffunc);

    unzFile uf = unzOpen2_64(path, &ffunc);
    CTASSERTF(uf != nullptr, "failed to open zip file %s", path);

    unz_global_info64 info;
    SM_ASSERT_UNZ(unzGetGlobalInfo64(uf, &info));

    for (uLong i = 0; i < info.number_entry; i++) {
        extract_dirent(uf, path);

        if ((i + 1) < info.number_entry) {
            SM_ASSERT_UNZ(unzGoToNextFile(uf));
        }
    }

    unzClose(uf);
}
