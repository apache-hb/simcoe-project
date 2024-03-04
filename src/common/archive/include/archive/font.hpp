#pragma once

#include "core/text.hpp"

#include "archive/image.hpp"

namespace sm::font {
    using Bounds = math::rectf;

    // TODO: should math::Vec2<T> have an h/v alias?
    struct Advance {
        float h;
        float v;
    };

    struct GlyphPair {
        char32 first;
        char32 second;

        constexpr auto operator<=>(const GlyphPair &other) const = default;
    };

    struct KerningPair {
        char32 first;
        char32 second;
        Advance advance;
    };

    struct Glyph {
        char32 codepoint;
        Bounds plane;
        Bounds image;
        Advance advance;
    };

    struct Metrics {
        float size;
        float range;

        float em_size;
        float ascender;
        float descender;
        float line_height;
        float underline_y;
        float underline_thickness;
    };

    struct Family {
        sm::String name;

        Metrics metrics;

        sm::Vector<Glyph> glyphs;
        sm::Vector<KerningPair> kerning;
    };

    struct FontInfo {
        sm::String name;
        sm::Vector<Family> families;
        ImageData image;
    };
}
