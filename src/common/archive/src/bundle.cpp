#include "archive/bundle.hpp"

#include "archive/io.hpp"

#include "core/arena.hpp"
#include "core/memory.hpp"
#include "core/text.hpp"
#include "core/format.hpp" // IWYU pragma: keep

#include "fs/fs.h"
#include "tar/tar.h"

using namespace sm;

Bundle::Bundle(fs_t *vfs, logs::ILogger &logger)
    : mSink(logger)
    , mFileSystem(vfs)
{ }

Bundle::Bundle(io_t *stream, archive::BundleFormat type, logs::ILogger &logger)
    : Bundle(fs_virtual("bundle", sm::global_arena()), logger)
{
    if (type != archive::BundleFormat::eTar) {
        mSink.warn("unsupported bundle format: {}", type);
        return;
    }

    if (tar_result_t err = tar_extract(*mFileSystem, stream); err.error != eTarOk) {
        mSink.error("failed to extract bundle: {}", tar_error_string(err.error));
    }
}

sm::Span<const uint8> Bundle::get_file(sm::StringView dir, sm::StringView name) const {
    sm::String path = fmt::format("bundle/{}/{}", dir, name);
    IoHandle file = fs_open(*mFileSystem, path.c_str(), eOsAccessRead);
    if (OsError err = io_error(*file); err.failed()) {
        mSink.error("failed to open file: {}", err);
        return {};
    }

    size_t size = io_size(*file);
    const uint8 *data = (uint8*)io_map(*file, eOsProtectRead);

    sm::Memory mem = size;
    mSink.info("loaded file: {} ({} bytes)", path, mem);

    return { data, size };
}

ShaderIr Bundle::get_shader_bytecode(const char *name) const {
    return get_file("shaders", fmt::format("{}.cso", name));
}

TextureData Bundle::get_texture(const char *name) const {
    return get_file("textures", fmt::format("{}.dds", name));
}
