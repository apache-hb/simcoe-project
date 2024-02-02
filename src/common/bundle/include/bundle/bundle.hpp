#pragma once

#include "core/unique.hpp"
#include "core/vector.hpp"

#include "logs/logs.hpp"

#include "core/map.hpp"
#include "core/text.hpp"
#include "core/macros.hpp"
#include "math/math.hpp"

#include "service/freetype.hpp" // IWYU pragma: export

#include "bundle.reflect.h" // IWYU pragma: export

#include <span>

typedef struct fs_t fs_t;

namespace sm::bundle {
    class AssetBundle;
    class Font;

    using AssetSink = logs::Sink<logs::Category::eAssets>;
    using BinaryData = sm::Vector<uint8_t>;
    using BinarySpan = std::span<const uint8_t>;

    struct Image {
        DataFormat format = DataFormat::eRGBA8_UINT;
        math::uint2 size;
        sm::UniqueArray<uint8_t> data;

        SM_NOCOPY(Image)

        Image(math::uint2 sz)
            : size(sz)
            , data(size_t(sz.width * sz.height * 4))
        { }

        Image(math::uint2 sz, DataFormat fmt, sm::UniqueArray<uint8_t>&& data)
            : format(fmt)
            , size(sz)
            , data(std::move(data))
        { }

        Image(Image&&) = default;

        uint8_t& get_pixel(size_t x, size_t y, size_t channel);
        math::uint8x4& get_pixel_rgba(size_t x, size_t y);
    };

    struct GlyphInfo {
        math::int2 size;
        math::int2 bearing;
    };

    struct FontInfo {
        float ascender;
        float descender;
    };

    class Font {
        friend class AssetBundle;

        static void delete_face(FT_Face face);
        using UniqueFace = sm::FnUniquePtr<FT_FaceRec, delete_face>;

        const AssetSink& m_log;
        const char *m_name;
        BinaryData m_data;
        UniqueFace m_face;

        FontInfo m_info;

        Font(const AssetSink& log, BinaryData data, const char *name);

    public:
        SM_NOCOPY(Font)

        Font(Font&&) = default;

        // TODO: dont do this
        const AssetSink& get_logger() const { return m_log; }
        const FontInfo& get_info() const { return m_info; }
        FT_Face get_face() const { return m_face.get(); }

        math::uint2 get_glyph_size(char32_t codepoint) const;

        GlyphInfo draw_glyph(char32_t codepoint, math::uint2 start, Image& image, const math::float4& color) const;
    };

    class AssetBundle {
        const char *m_path;

        AssetSink m_log;
        fs_t *m_vfs;

        sm::Map<sm::String, BinaryData> m_shaders;
        sm::Map<sm::String, Font> m_fonts;
        sm::Map<sm::String, Image> m_images;

        void open_fs();

    public:
        SM_NOCOPY(AssetBundle)
        SM_NOMOVE(AssetBundle)

        AssetBundle(const char *path, logs::ILogger& logger)
            : m_path(path)
            , m_log(logger)
        {
            open_fs();
        }

        BinarySpan load_shader(const char *path);
        Font& load_font(const char *path);
        const Image& load_image(const char *path, math::uint2 size);
    };
}
