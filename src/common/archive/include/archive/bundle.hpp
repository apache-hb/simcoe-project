#pragma once

#include "core/core.hpp"

#include "core/span.hpp"
#include "core/text.hpp"
#include "logs/sink.hpp"

#include "fs.hpp"

#include "io.reflect.h"

typedef struct fs_t fs_t;
typedef struct io_t io_t;

namespace sm {
    using AssetSink = logs::Sink<logs::Category::eAssets>;

    using ShaderIr = sm::Span<const uint8>;
    using TextureData = sm::Span<const uint8>;

    class Bundle {
        AssetSink mSink;
        FsHandle mFileSystem;

        sm::Span<const uint8> get_file(sm::StringView dir, sm::StringView name) const;

    public:
        Bundle(fs_t *vfs, logs::ILogger &logger);
        Bundle(io_t *stream, archive::BundleFormat type, logs::ILogger &logger);

        ShaderIr get_shader_bytecode(const char *name) const;
        TextureData get_texture(const char *name) const;
    };
}
