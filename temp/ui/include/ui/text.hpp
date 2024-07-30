#pragma once

#include "core/macros.hpp"
#include "core/memory/unique.hpp"
#include "core/utf8.hpp"

#include "bundle/bundle.hpp"

#include <hb.h>

namespace sm::ui {
    struct ShapedGlyph {
        hb_codepoint_t codepoint;
        hb_position_t x_advance;
        hb_position_t y_advance;
        hb_position_t x_offset;
        hb_position_t y_offset;
    };

    struct ShapedTextIterator {
        unsigned int m_index;
        hb_glyph_info_t *m_glyph_info;
        hb_glyph_position_t *m_glyph_position;

    public:
        ShapedTextIterator(const ShapedTextIterator& other) = default;
        ShapedTextIterator(unsigned int index, hb_glyph_info_t *glyph_info, hb_glyph_position_t *glyph_position);

        bool operator==(const ShapedTextIterator& other) const;
        bool operator!=(const ShapedTextIterator& other) const;

        ShapedGlyph operator*() const;
        ShapedTextIterator& operator++();
    };

    class ShapedText {
        using UniqueHB = sm::FnUniquePtr<hb_buffer_t, hb_buffer_destroy>;
        UniqueHB m_buffer;

        unsigned int m_glyph_count;
        hb_glyph_info_t *m_glyph_info;
        hb_glyph_position_t *m_glyph_position;

    public:
        SM_NOCOPY(ShapedText)

        ShapedText(hb_buffer_t *buffer);

        ShapedTextIterator begin() const;
        ShapedTextIterator end() const;
    };

    class TextShaper {
        using UniqueFont = sm::FnUniquePtr<hb_font_t, hb_font_destroy>;

        UniqueFont m_font;
        hb_face_t *m_face;

    public:
        SM_NOCOPY(TextShaper)

        TextShaper(bundle::Font& font);

        ShapedText shape(utf8::StaticText text) const;
    };
}
