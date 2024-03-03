#pragma once

#include "core/core.hpp"

#include "core/span.hpp"
#include "core/text.hpp"
#include "core/vector.hpp"

#include "fs.hpp"

#include "io.reflect.h"

typedef struct fs_t fs_t;
typedef struct io_t io_t;

namespace sm {
    using ShaderIr = sm::Span<const uint8>;
    using TextureData = sm::Span<const uint8>;

    struct Bounds {
        float left;
        float top;
        float right;
        float bottom;
    };

    struct Advance {
        float horizontal;
        float vertical;
    };

    struct FontInfo {
        struct KerningPair {
            char32_t first;
            char32_t second;
            Advance advance;
        };

        struct Glyph {
            char32_t codepoint;
            Bounds plane;
            Bounds texture;
            Advance advance;
        };

        sm::String name;
        sm::Vector<Glyph> glyphs;
        sm::Vector<KerningPair> kerning;
        TextureData texture;
    };

    class Bundle {
        FsHandle mFileSystem;

        sm::Span<const uint8> get_file(sm::StringView dir, sm::StringView name) const;

    public:
        Bundle(fs_t *vfs);
        Bundle(io_t *stream, archive::BundleFormat type);

        ShaderIr get_shader_bytecode(const char *name) const;
        TextureData get_texture(const char *name) const;
        FontInfo get_font(const char *name) const;
    };
}
