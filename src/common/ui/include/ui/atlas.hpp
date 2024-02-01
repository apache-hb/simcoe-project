#pragma once

#include "math/math.hpp"

#include "ui/text.hpp"

namespace sm::ui {
    struct GlyphInfo {
        math::float2 uv0;
        math::float2 uv1;

        math::int2 size;
        math::int2 bearing;
    };

    class FontAtlas {
        bundle::Font& m_font;
        TextShaper m_shaper;

        // render info
        bundle::Image m_image;

        // uv coords for a 2x2 white pixel
        math::float2 m_white_pixel_uv;

    public:
        SM_NOCOPY(FontAtlas)

        // lookup info
        sm::Map<char32_t, GlyphInfo> glyphs;

        FontAtlas(bundle::Font& font, math::uint2 size, std::span<const char32_t> codepoints);

        const bundle::Image& get_image() const { return m_image; }
        const TextShaper& get_shaper() const { return m_shaper; }
        const bundle::FontInfo& get_font_info() const { return m_font.get_info(); }
        const math::float2& get_white_pixel_uv() const { return m_white_pixel_uv; }
        const GlyphInfo& get_glyph_coords(char32_t codepoint) const { return glyphs.at(codepoint); }
    };
}
