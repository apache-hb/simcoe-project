#pragma once

#include "core/core.hpp"

#include "core/span.hpp"
#include "core/string.hpp"
#include "core/unique.hpp"

#include "archive/fs.hpp"

typedef struct fs_t fs_t;
typedef struct io_t io_t;

namespace sm {
    using ShaderIr = sm::View<byte>;
    using TextureData = sm::View<byte>;

    class Bundle {
        sm::UniquePtr<IFileSystem> mFileSystem;

        sm::View<byte> getFileData(sm::StringView dir, sm::StringView name);

    public:
        Bundle(IFileSystem *fs);

        ShaderIr get_shader_bytecode(const char *name);
        TextureData get_texture(const char *name);

        // font::FontInfo get_font(const char *name);
    };
}
