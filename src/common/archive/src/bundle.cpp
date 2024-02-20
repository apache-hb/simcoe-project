#include "archive/bundle.hpp"

#include "archive/io.hpp"

#include "core/arena.hpp"
#include "core/text.hpp"
#include "core/format.hpp" // IWYU pragma: keep

#include "fs/fs.h"
#include "tar/tar.h"

using namespace sm;

Bundle::Bundle(fs_t *vfs, logs::ILogger &logger)
    : mSink(logger)
    , mFiles(vfs)
{ }

Bundle::Bundle(io_t *stream, archive::BundleFormat type, logs::ILogger &logger)
    : Bundle(fs_virtual("bundle", sm::global_arena()), logger)
{
    if (type != archive::BundleFormat::eTar) {
        mSink.warn("Unsupported bundle format: {}", type);
        return;
    }

    if (tar_result_t err = tar_extract(mFiles, stream); err.error != eTarOk) {
        mSink.error("Failed to extract bundle: {}", tar_error_string(err.error));
    }

    fs_iter_dirents(mFiles, ".", nullptr, [](const char *dir, const char *name, os_dirent_t type, void *ctx) {
        fmt::println("{}/{} {}", dir, name, os_dirent_string(type));
    });
}

ShaderIr Bundle::get_shader_bytecode(const char *name) const {
    sm::String path = fmt::format("bundle/shaders/{}.cso", name);
    IoHandle file = fs_open(mFiles, path.c_str(), eOsAccessRead);
    if (OsError err = io_error(*file); err.failed()) {
        mSink.error("Failed to open shader: {}", err);
        return {};
    }

    size_t size = io_size(*file);
    const uint8 *data = (uint8*)io_map(*file, eOsProtectRead);

    mSink.info("Loaded shader: {} ({} bytes)", path, size);

    return { data, size };
}
