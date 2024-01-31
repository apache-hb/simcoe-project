#include "ui/text.hpp"

#include "hb-ft.h"

using namespace sm;
using namespace sm::ui;

// harfbuzz

ShapedTextIterator::ShapedTextIterator(unsigned int index, hb_glyph_info_t *glyph_info, hb_glyph_position_t *glyph_position)
    : m_index(index)
    , m_glyph_info(glyph_info)
    , m_glyph_position(glyph_position)
{ }

bool ShapedTextIterator::operator==(const ShapedTextIterator& other) const {
    return m_index == other.m_index;
}

bool ShapedTextIterator::operator!=(const ShapedTextIterator& other) const {
    return m_index != other.m_index;
}

ShapedGlyph ShapedTextIterator::operator*() const {
    return {
        .codepoint = m_glyph_info[m_index].codepoint,
        .x_advance = m_glyph_position[m_index].x_advance >> 6,
        .y_advance = m_glyph_position[m_index].y_advance >> 6,
        .x_offset = m_glyph_position[m_index].x_offset >> 6,
        .y_offset = m_glyph_position[m_index].y_offset >> 6
    };
}

ShapedTextIterator& ShapedTextIterator::operator++() {
    m_index++;
    return *this;
}

ShapedText::ShapedText(hb_buffer_t *buffer)
    : m_buffer(buffer)
    , m_glyph_count(hb_buffer_get_length(m_buffer))
    , m_glyph_info(hb_buffer_get_glyph_infos(m_buffer, nullptr))
    , m_glyph_position(hb_buffer_get_glyph_positions(m_buffer, nullptr))
{ }

ShapedText::ShapedText(ShapedText&& other) noexcept
    : m_buffer(other.m_buffer)
    , m_glyph_count(other.m_glyph_count)
    , m_glyph_info(other.m_glyph_info)
    , m_glyph_position(other.m_glyph_position)
{
    other.m_buffer = nullptr;
    other.m_glyph_count = 0;
    other.m_glyph_info = nullptr;
    other.m_glyph_position = nullptr;
}

ShapedText::~ShapedText() {
    if (m_buffer) hb_buffer_destroy(m_buffer);
}

ShapedTextIterator ShapedText::begin() const {
    return { 0, m_glyph_info, m_glyph_position };
}

ShapedTextIterator ShapedText::end() const {
    return { m_glyph_count, m_glyph_info, m_glyph_position };
}

TextShaper::TextShaper(bundle::Font& font) {
    hb_font_t *new_font = hb_ft_font_create_referenced(font.get_face());
    hb_face_t *new_face = hb_font_get_face(new_font);

    hb_ft_font_set_funcs(new_font);
    hb_face_set_upem(new_face, 64);

    m_font = new_font;
    m_face = new_face;
}

TextShaper::~TextShaper() {
    CTASSERTF(m_font != nullptr, "font was not destroyed");
    hb_font_destroy(m_font);
    m_font = nullptr;
}

ShapedText TextShaper::shape(utf8::StaticText text) const {
    hb_buffer_t *buffer = hb_buffer_create();
    hb_buffer_add_utf8(buffer, (const char*)text.data(), int(text.size()), 0, int(text.size()));

    hb_buffer_set_direction(buffer, HB_DIRECTION_LTR);
    hb_buffer_set_script(buffer, HB_SCRIPT_LATIN);
    hb_buffer_set_language(buffer, hb_language_from_string("en", -1));

    hb_shape(m_font, buffer, nullptr, 0);

    return ShapedText(buffer);
}
