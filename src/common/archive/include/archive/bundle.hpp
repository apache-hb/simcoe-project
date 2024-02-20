#pragma once

#include "core/core.hpp"
#include "core/macros.hpp"

#include "core/span.hpp"
#include "logs/sink.hpp"

#include "io.reflect.h"

typedef struct fs_t fs_t;
typedef struct io_t io_t;

namespace sm {
    using AssetSink = logs::Sink<logs::Category::eAssets>;

    using ShaderIr = sm::Span<const uint8>;

    class Bundle {
        AssetSink mSink;
        fs_t *mFiles;

    public:
        SM_NOCOPY(Bundle)

        Bundle(fs_t *vfs, logs::ILogger &logger);
        Bundle(io_t *stream, archive::BundleFormat type, logs::ILogger &logger);

        constexpr Bundle(Bundle&& other)
            : mSink(other.mSink)
            , mFiles(other.mFiles)
        {
            other.mFiles = nullptr;
        }

        ShaderIr get_shader_bytecode(const char *name) const;
    };
}
