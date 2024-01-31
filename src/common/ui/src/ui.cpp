#include "ui/ui.hpp"

using namespace sm;
using namespace sm::ui;

static math::float2 adjust_bounds(const BoxBounds& item, const BoxBounds& bounds, Align align) {
    using math::float2;

    float2 size = item.max - item.min;
    float2 offset = bounds.min;

    switch (align.h) {
    case AlignH::eLeft:
        offset.x = bounds.min.x;
        break;
    case AlignH::eCenter:
        offset.x = bounds.min.x + (bounds.max.x - bounds.min.x - size.x) / 2.f;
        break;
    case AlignH::eRight:
        offset.x = bounds.max.x - size.x;
        break;
    }

    switch (align.v) {
    case AlignV::eTop:
        offset.y = bounds.min.y;
        break;
    case AlignV::eMiddle:
        offset.y = bounds.min.y + (bounds.max.y - bounds.min.y - size.y) / 2.f;
        break;
    case AlignV::eBottom:
        offset.y = bounds.max.y - size.y;
        break;
    }

    return offset;
}

void LayoutInfo::box(const BoxBounds& bounds, const math::uint8x4& colour) {
    const auto& white = canvas.get_white_pixel_uv();
    Vertex vertices[4] = {
        { bounds.min, white, colour },
        { { bounds.max.x, bounds.min.y }, white, colour },
        { { bounds.min.x, bounds.max.y }, white, colour },
        { bounds.max, white, colour },
    };

    size_t offset = draw.vertices.size();

    Index indices[6] = {
        Index(0u + offset), Index(1u + offset), Index(2u + offset),
        Index(2u + offset), Index(1u + offset), Index(3u + offset),
    };

    draw.vertices.insert(draw.vertices.end(), std::begin(vertices), std::end(vertices));
    draw.indices.insert(draw.indices.end(), std::begin(indices), std::end(indices));
}

static constexpr math::uint8x4 kTextColour = { 0, 0, 0, 255 };

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

        float2 glyph_size = size.as<float>();
        float2 glyph_start = cursor + float2{ fxo, fyo };

        float2 top_left = glyph_start + float2{0.f, glyph_size.y};
        float2 bottom_right = glyph_start + float2{glyph_size.x, 0.f};

        Vertex vertices[4] = {
            // top left
            { { glyph_start.x, glyph_start.y + glyph_size.y }, { tex_min.x, tex_min.y }, kTextColour },
            // top right
            { glyph_start + glyph_size, { tex_max.x, tex_min.y }, kTextColour },
            // bottom left
            { glyph_start, { tex_min.x, tex_max.y }, kTextColour },
            // bottom right
            { { glyph_start.x + glyph_size.x, glyph_start.y }, { tex_max.x, tex_max.y }, kTextColour },
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

    float2 offset = adjust_bounds({ text_min, text_max }, bounds, get_align());

    for (size_t i = start; i < index; ++i) {
        info.draw.vertices[i].position += offset;
    }
}

void Canvas::layout(const IWidget& widget) {
    m_draw.indices.clear();
    m_draw.vertices.clear();

    LayoutInfo info = { m_draw, *this };
    widget.layout(info, m_screen);
    m_dirty = true;
}
