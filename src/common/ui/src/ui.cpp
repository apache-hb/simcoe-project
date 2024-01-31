#include "ui/ui.hpp"

using namespace sm;
using namespace sm::ui;

static math::float2 adjust_bounds(const BoxBounds& item, const BoxBounds& bounds, Align align) {
    using math::float2;

    // max is the bottom right
    // min is the top left
    // max gets larger as you go right and down

    // the item may be outside the bounds so nudge it back in when aligning

    float2 offset = 0.f;

    switch (align.h) {
    case AlignH::eLeft:
        offset.x = bounds.min.x;
        break;
    case AlignH::eCenter:
        offset.x = bounds.min.x + (bounds.max.x - bounds.min.x) / 2.f - (item.min.x + item.max.x) / 2.f;
        break;
    case AlignH::eRight:
        offset.x = bounds.max.x - item.max.x;
        break;
    }

    switch (align.v) {
    case AlignV::eTop:
        offset.y = bounds.max.y - item.max.y;
        break;
    case AlignV::eMiddle:
        offset.y = bounds.min.y + (bounds.max.y - bounds.min.y) / 2.f - (item.min.y + item.max.y) / 2.f;
        break;
    case AlignV::eBottom:
        offset.y = bounds.min.y;
        break;
    }

    return offset;
}

void LayoutInfo::box(const BoxBounds& bounds, const math::uint8x4& colour) {
    if (bounds.min.x > bounds.max.x)
        std::printf("min x is greater than max x %f %f\n", bounds.min.x, bounds.max.x);

    if (bounds.min.y > bounds.max.y)
        std::printf("min y is greater than max y %f %f\n", bounds.min.y, bounds.max.y);

    using math::float2;

    float2 min = math::min(bounds.min, bounds.max);
    float2 max = math::max(bounds.min, bounds.max);

    const auto& white_uv = canvas.get_white_pixel_uv();
    Vertex vertices[4] = {
        // top left
        { min, white_uv, colour },

        // top right
        { { min.x, max.y }, white_uv, colour },

        // bottom left
        { { max.x, min.y }, white_uv, colour },

        // bottom right
        { max, white_uv, colour },
    };

    size_t offset = draw.vertices.size();

    Index indices[6] = {
        Index(0u + offset), Index(1u + offset), Index(2u + offset),
        Index(2u + offset), Index(1u + offset), Index(3u + offset),
    };

    draw.vertices.insert(draw.vertices.end(), std::begin(vertices), std::end(vertices));
    draw.indices.insert(draw.indices.end(), std::begin(indices), std::end(indices));
}

void LayoutInfo::rect(const BoxBounds& bounds, float border, const math::uint8x4& colour) {
    // draw outline box made of 4 lines

    // top
    box({ bounds.min, { bounds.max.x, bounds.min.y + border } }, colour);
    // bottom
    box({ { bounds.min.x, bounds.max.y - border }, bounds.max }, colour);
    // left
    box({ bounds.min, { bounds.min.x + border, bounds.max.y } }, colour);
    // right
    box({ { bounds.max.x - border, bounds.min.y }, bounds.max }, colour);
}

void TextWidget::layout(LayoutInfo& info, BoxBounds bounds) const {
    using math::float2;

    auto& shaper = m_font.get_shaper();
    auto shaped = shaper.shape(m_text);

    auto tbegin = m_text.begin();
    auto tend = m_text.end();

    auto gbegin = shaped.begin();
    auto gend = shaped.end();

    // relative to top left of text
    float2 cursor = 0.f;
    size_t start = info.draw.vertices.size();
    size_t index = info.draw.vertices.size();

    // min and max extents of text relative to the bounding box
    float2 text_min = 0.f;
    float2 text_max = 0.f;

    for (; gbegin != gend && tbegin != tend; ++gbegin, ++tbegin) {
        const auto& glyph = *gbegin;
        const auto& codepoint = *tbegin;

        float fxo = float(glyph.x_offset);
        float fyo = float(glyph.y_offset);
        float fxa = float(glyph.x_advance);
        float fya = float(glyph.y_advance);

        auto it = m_font.glyphs.find(codepoint);
        if (it == m_font.glyphs.end()) {
            cursor.x += fxa;
            cursor.y += fya;
            continue;
        }

        auto [tex_min, tex_max, size] = it->second;

        if (fxa == 0.f) {
            fxa = float(size.x);
        }

        // harfbuzz gives us inverted uv along the v axis
        // fix them here
        float2 glyph_size = size.as<float>();
        float2 glyph_start = cursor + float2{ fxo, fyo };

        float2 top_left = glyph_start;
        float2 bottom_right = glyph_start + glyph_size;

        Vertex vertices[4] = {
            // top left
            { top_left, {tex_min.u, tex_max.v}, kColourWhite },

            // top right
            { { top_left.x, bottom_right.y }, { tex_min.u, tex_min.v }, kColourWhite },

            // bottom left
            { { bottom_right.x, top_left.y }, { tex_max.u, tex_max.v }, kColourWhite },

            // bottom right
            { bottom_right, { tex_max.u, tex_min.v }, kColourWhite },
        };

        Index idx = Index(index);

        Index indices[6] = {
            Index(0u + idx), Index(1u + idx), Index(2u + idx),
            Index(2u + idx), Index(1u + idx), Index(3u + idx),
        };

        index += 4;

        text_min = math::min(text_min, top_left);
        text_max = math::max(text_max, bottom_right);

        info.draw.vertices.insert(info.draw.vertices.end(), std::begin(vertices), std::end(vertices));
        info.draw.indices.insert(info.draw.indices.end(), std::begin(indices), std::end(indices));

        cursor.x += fxa;
        cursor.y += fya;
    }

    BoxBounds text_bounds = { text_min, text_max };

    // bounds are relative to the top left of the text
    float2 offset = adjust_bounds(text_bounds, bounds, get_align());

    for (size_t i = start; i < index; i++) {
        info.draw.vertices[i].position += offset;
    }

    info.rect(bounds, 3.f, kColourGreen);
    info.rect({ text_min + offset, text_max + offset }, 3.f, kColourBlue);
}

void Canvas::layout(const IWidget& widget) {
    m_draw.indices.clear();
    m_draw.vertices.clear();

    LayoutInfo info = { m_draw, *this };
    widget.layout(info, get_user());
    m_dirty = true;
}
