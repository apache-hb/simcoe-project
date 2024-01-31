#include "ui/ui.hpp"

using namespace sm;
using namespace sm::ui;

using namespace math;

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

void LayoutResult::offset(CanvasDrawData& draw, math::float2 offset) {
    bounds.min += offset;
    bounds.max += offset;

    for (size_t i = vtx_offset; i < vtx_offset + vtx_count; i++) {
        draw.vertices[i].position += offset;
    }
}

LayoutResult IWidget::layout(LayoutInfo& info, BoxBounds bounds) const {
    size_t vtx_offset = info.draw.vertices.size();
    size_t idx_offset = info.draw.indices.size();

    BoxBounds item = impl_layout(info, bounds);

    if (draw_bounds()) {
        info.rect(item, 3.f, dbg_colour());
    }

    size_t vtx_count = info.draw.vertices.size() - vtx_offset;
    size_t idx_count = info.draw.indices.size() - idx_offset;

    return {
        .bounds = item,
        .vtx_offset = vtx_offset,
        .idx_offset = idx_offset,
        .vtx_count = vtx_count,
        .idx_count = idx_count,
    };
}

void LayoutInfo::box(const BoxBounds& bounds, const math::uint8x4& colour) {
    if (bounds.min.x > bounds.max.x)
        std::printf("min x is greater than max x %f %f\n", bounds.min.x, bounds.max.x);

    if (bounds.min.y > bounds.max.y)
        std::printf("min y is greater than max y %f %f\n", bounds.min.y, bounds.max.y);

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

BoxBounds TextWidget::impl_layout(LayoutInfo& info, BoxBounds bounds) const {
    auto& shaper = m_font.get_shaper();
    const auto& metrics = m_font.get_font_info();
    auto shaped = shaper.shape(m_text);

    auto tbegin = m_text.begin();
    auto tend = m_text.end();

    auto gbegin = shaped.begin();
    auto gend = shaped.end();

    // relative to top left of text
    float2 cursor = 0.f;
    size_t vtx_start = info.draw.vertices.size();
    size_t vtx_offset = info.draw.vertices.size();

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

        auto [tex_min, tex_max, bearing, size] = it->second;

        if (fxa == 0.f) {
            fxa = float(size.x);
        }

        // harfbuzz gives us inverted uv along the v axis
        // fix them here
        float2 glyph_size = size.as<float>();
        float2 glyph_start = cursor + float2{ fxo, fyo };

        float2 top_left = glyph_start;
        float2 bottom_right = glyph_start + glyph_size;

        // get the difference between height and bearing to get the y offset
        // harfbuzz handles everything else for us

        float y_offset = float(bearing.y - size.y);
        top_left.y += y_offset;
        bottom_right.y += y_offset;

        // apply scale
        top_left *= m_scale;
        bottom_right *= m_scale;

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

        Index idx = Index(vtx_offset);

        Index indices[6] = {
            Index(0u + idx), Index(1u + idx), Index(2u + idx),
            Index(2u + idx), Index(1u + idx), Index(3u + idx),
        };

        vtx_offset += 4;

        text_min = math::min(text_min, top_left);
        text_max = math::max(text_max, bottom_right);

        info.draw.vertices.insert(info.draw.vertices.end(), std::begin(vertices), std::end(vertices));
        info.draw.indices.insert(info.draw.indices.end(), std::begin(indices), std::end(indices));

        cursor.x += fxa;
        cursor.y += fya;
    }

    // apply metrics ascender and descender
    text_min.y += metrics.ascender * m_scale;
    text_max.y += metrics.ascender * m_scale;

    text_min.y -= metrics.descender * m_scale;
    text_max.y -= metrics.descender * m_scale;

    BoxBounds text_bounds = { text_min, text_max };

    // bounds are relative to the top left of the text
    float2 offset = adjust_bounds(text_bounds, bounds, get_align());

    for (size_t i = vtx_start; i < vtx_offset; i++) {
        info.draw.vertices[i].position += offset;
    }

    BoxBounds offset_bounds = { text_bounds.min + offset, text_bounds.max + offset };

    return offset_bounds;
}

BoxBounds VStackWidget::impl_layout(LayoutInfo& info, BoxBounds bounds) const {
    size_t count = m_children.size();
    // split the bounds into equal vertical slices

    Padding pad = get_padding();

    float height = (bounds.max.y - bounds.min.y) - (pad.top + pad.bottom);

    // also account for padding between the children
    float padding = (float(count) - 1) * m_spacing;

    float child_height = (height - padding) / float(count);
    float y = bounds.max.y - pad.top;

    LayoutResultList results;

    for (auto& child : m_children) {
        BoxBounds child_bounds = {
            { bounds.min.x + pad.left, y - child_height },
            { bounds.max.x - pad.right, y },
        };

        results.push_back(child->layout(info, child_bounds));

        y -= child_height + m_spacing;
    }

    // calculate the bounds of the entire layout
    BoxBounds layout_bounds = { bounds.min, bounds.max };

    for (auto& result : results) {
        layout_bounds.min = math::min(layout_bounds.min, result.bounds.min);
        layout_bounds.max = math::max(layout_bounds.max, result.bounds.max);
    }

    return layout_bounds;
}

void HStackWidget::justify_left(CanvasDrawData& draw, LayoutResultSpan results, const BoxBounds& bounds) const {
    float2 min = bounds.min;

    // just start placing the children from the left
    for (auto& result : results) {
        BoxBounds result_bounds = result.bounds;
        result_bounds.max.x = min.x + (result_bounds.max.x - result_bounds.min.x);
        result_bounds.min.x = min.x;

        result.offset(draw, result_bounds.min - result.bounds.min);

        min.x = result_bounds.max.x + m_spacing;
    }
}

void HStackWidget::justify_right(CanvasDrawData& draw, LayoutResultSpan results, const BoxBounds& bounds) const {
    float2 max = bounds.max;

    // just start placing the children from the right
    for (auto& result : results) {
        BoxBounds result_bounds = result.bounds;
        result_bounds.min.x = max.x - (result_bounds.max.x - result_bounds.min.x);
        result_bounds.max.x = max.x;

        result.offset(draw, result_bounds.min - result.bounds.min);

        max.x = result_bounds.min.x - m_spacing;
    }
}

void HStackWidget::justify_center(CanvasDrawData& draw, LayoutResultSpan results, const BoxBounds& bounds) const {
    // total width needed for all children
    float total_width = 0.f;
    for (auto& result : results) {
        total_width += result.bounds.max.x - result.bounds.min.x;
    }

    // the space between each child
    float spacing = m_spacing * float(results.size() - 1);

    // the space left over on the left and right
    float left_over = bounds.max.x - bounds.min.x - total_width - spacing;

    // the offset to the left
    float offset = left_over / 2.f;

    float2 min = bounds.min;

    for (auto& result : results) {
        BoxBounds result_bounds = result.bounds;
        result_bounds.min.x = min.x + offset;
        result_bounds.max.x = min.x + offset + (result_bounds.max.x - result_bounds.min.x);

        result.offset(draw, result_bounds.min - result.bounds.min);

        min.x = result_bounds.max.x + m_spacing;
    }
}

BoxBounds HStackWidget::impl_layout(LayoutInfo& info, BoxBounds bounds) const {
    size_t count = m_children.size();
    // split the bounds into equal horizontal slices

    Padding pad = get_padding();

    float width = (bounds.max.x - bounds.min.x) - (pad.left + pad.right);

    // also account for padding between the children
    float padding = (float(count) - 1) * m_spacing;

    float child_width = (width - padding) / float(count);
    float x = bounds.min.x + pad.left;

    LayoutResultList results;

    for (auto& child : m_children) {
        BoxBounds child_bounds = {
            { x, bounds.min.y + pad.bottom },
            { x + child_width, bounds.max.y - pad.top },
        };

        results.push_back(child->layout(info, child_bounds));

        x += child_width + m_spacing;
    }

    auto align = get_align();

    if (m_justify == Justify::eFollowAlign) {
        switch (align.h) {
        case AlignH::eLeft:
            justify_left(info.draw, results, bounds);
            break;
        case AlignH::eCenter:
            justify_center(info.draw, results, bounds);
            break;
        case AlignH::eRight:
            justify_right(info.draw, results, bounds);
            break;
        }
    }

    switch (align.v) {
    case AlignV::eTop:
        for (auto& result : results) {
            result.offset(info.draw, { 0.f, bounds.max.y - result.bounds.max.y });
        }
        break;
    case AlignV::eMiddle:
        for (auto& result : results) {
            float2 offset = { 0.f, (bounds.max.y - bounds.min.y) / 2.f - (result.bounds.max.y - result.bounds.min.y) / 2.f };
            result.offset(info.draw, offset);
        }
        break;
    case AlignV::eBottom:
        for (auto& result : results) {
            result.offset(info.draw, { 0.f, bounds.min.y - result.bounds.min.y });
        }
        break;
    }

    // calculate the bounds of the entire layout

    // max and min are swapped on purpose to get the correct bounds
    BoxBounds layout_bounds = { bounds.max, bounds.min };

    for (auto& result : results) {
        layout_bounds.min = math::min(layout_bounds.min, result.bounds.min);
        layout_bounds.max = math::max(layout_bounds.max, result.bounds.max);
    }

    return layout_bounds;
}

void Canvas::layout(const IWidget& widget) {
    m_draw.indices.clear();
    m_draw.vertices.clear();

    LayoutInfo info = { m_draw, *this };
    widget.layout(info, get_user());
    m_dirty = true;
}
