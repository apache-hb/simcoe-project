#pragma once

#include "math/math.hpp"

#include "ui/text.hpp"

namespace sm::ui {
    struct GlyphCoords {
        math::float2 uv0;
        math::float2 uv1;

        // TODO: this feels wrong
        math::uint2 size;
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

        FontAtlas(FontAtlas&& other)
            : m_font(other.m_font)
            , m_shaper(std::move(other.m_shaper))
            , m_image(std::move(other.m_image))
            , m_white_pixel_uv(other.m_white_pixel_uv)
            , glyphs(std::move(other.glyphs))
        { }

        // lookup info
        sm::Map<char32_t, GlyphCoords> glyphs;

        FontAtlas(bundle::Font& font, math::uint2 size, std::span<const char32_t> codepoints);

        const bundle::Image& get_image() const { return m_image; }
        const TextShaper& get_shaper() const { return m_shaper; }
        const math::float2& get_white_pixel_uv() const { return m_white_pixel_uv; }
        const GlyphCoords& get_glyph_coords(char32_t codepoint) const { return glyphs.at(codepoint); }
    };
}
