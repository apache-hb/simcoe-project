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

    std::printf("bounds %f %f %f %f\n", bounds.min.x, bounds.min.y, bounds.max.x, bounds.max.y);
    std::printf("item %f %f %f %f\n", item.min.x, item.min.y, item.max.x, item.max.y);

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
        offset.y = bounds.min.y;
        break;
    case AlignV::eMiddle:
        offset.y = bounds.min.y + (bounds.max.y - bounds.min.y) / 2.f - (item.min.y + item.max.y) / 2.f;
        break;
    case AlignV::eBottom:
        offset.y = bounds.max.y - item.max.y;
        break;
    }

    std::printf("offset %f %f\n", offset.x, offset.y);

    return offset;
}

void LayoutInfo::box(const BoxBounds& bounds, const math::uint8x4& colour) {
    CTASSERTF(bounds.min.x <= bounds.max.x, "min x is greater than max x %f %f", bounds.min.x, bounds.max.x);
    CTASSERTF(bounds.min.y <= bounds.max.y, "min y is greater than max y %f %f", bounds.min.y, bounds.max.y);

    const auto& white_uv = canvas.get_white_pixel_uv();
    Vertex vertices[4] = {
        // top left
        { bounds.min, white_uv, colour },

        // top right
        { { bounds.min.x, bounds.max.y }, white_uv, colour },

        // bottom left
        { { bounds.max.x, bounds.min.y }, white_uv, colour },

        // bottom right
        { bounds.max, white_uv, colour },
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
    float2 text_min = bounds.max;
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

        float2 glyph_size = size.as<float>();
        float2 glyph_start = cursor + float2{ fxo, fyo };

        // harfbuzz gives us the bottom left of the glyph as the origin
        // we use top left so we need to convert them to our coordinate system

        float2 top_left = { glyph_start.x, glyph_start.y + glyph_size.y };
        float2 bottom_right = { glyph_start.x + glyph_size.x, glyph_start.y };

        Vertex vertices[4] = {
            // top left
            { top_left, tex_min, m_colour },
            // top right
            { glyph_start + glyph_size, { tex_max.x, tex_min.y }, m_colour },
            // bottom left
            { glyph_start, { tex_min.x, tex_max.y }, m_colour },
            // bottom right
            { bottom_right, tex_max, m_colour },
        };

        Index idx = Index(index);

        Index indices[6] = {
            Index(0u + idx), Index(1u + idx), Index(2u + idx),
            Index(2u + idx), Index(1u + idx), Index(3u + idx),
        };

        index += 4;

        text_min = math::min(text_min, glyph_start - float2{ 0.f, glyph_size.y });
        text_max = math::max(text_max, glyph_size + glyph_start);

        info.draw.vertices.insert(info.draw.vertices.end(), std::begin(vertices), std::end(vertices));
        info.draw.indices.insert(info.draw.indices.end(), std::begin(indices), std::end(indices));

        cursor.x += fxa;
        cursor.y += fya;
    }

    // bounds are relative to the top left of the text
    float2 offset = adjust_bounds({ text_min, text_max }, bounds, get_align());

    for (size_t i = start; i < index; i++) {
        info.draw.vertices[i].position += offset;
    }

    info.rect(bounds, 3.f, kColourBlack);
}

void Canvas::layout(const IWidget& widget) {
    m_draw.indices.clear();
    m_draw.vertices.clear();

    LayoutInfo info = { m_draw, *this };
    widget.layout(info, get_screen());
    m_dirty = true;
}
