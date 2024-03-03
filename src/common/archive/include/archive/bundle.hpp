#pragma once

#include "core/core.hpp"

#include "core/span.hpp"
#include "core/text.hpp"

#include "fs.hpp"

#include "io.reflect.h"

typedef struct fs_t fs_t;
typedef struct io_t io_t;

namespace sm {
    using ShaderIr = sm::Span<const uint8>;
    using TextureData = sm::Span<const uint8>;

    class Bundle {
        FsHandle mFileSystem;

        sm::Span<const uint8> get_file(sm::StringView dir, sm::StringView name) const;

    public:
        Bundle(fs_t *vfs);
        Bundle(io_t *stream, archive::BundleFormat type);

        ShaderIr get_shader_bytecode(const char *name) const;
        TextureData get_texture(const char *name) const;
    };
}
