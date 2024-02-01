#include "ui/atlas.hpp"

#include "core/unique.hpp"
#include "stb/stb_rectpack.h"

using namespace sm;
using namespace sm::ui;

static constexpr math::float4 kWhiteFloat = { 1.0f, 1.0f, 1.0f, 1.0f };
static constexpr math::uint8x4 kWhite = { 255, 255, 255, 255 };

static constexpr char32_t kInvalidCodepoint = 0xFFFD;

FontAtlas::FontAtlas(bundle::Font& font, math::uint2 size, std::span<const char32_t> codepoints)
    : m_font(font)
    , m_shaper(font)
    , m_image(size)
{
    size_t len = codepoints.size();
    sm::UniquePtr<stbrp_node[]> nodes = len + 1;
    sm::UniquePtr<stbrp_rect[]> rects = len + 1;
    stbrp_context context;

    stbrp_init_target(&context, int(size.width), int(size.height), nodes.get(), int(len));

    for (size_t i = 0; i < len; i++) {
        auto [w, h] = m_font.get_glyph_size(codepoints[i]);
        rects[i] = { .id = int(codepoints[i]), .w = int(w) + 2, .h = int(h) + 2 };
    }

    // we pack in an extra 2x2 to store a white pixel
    rects[len] = { .id = kInvalidCodepoint, .w = 2, .h = 2 };

    int ret = stbrp_pack_rects(&context, rects.get(), int(len));
    CTASSERTF(ret == 1, "failed to pack rects into atlas");

    // render every glyph into the atlas

    for (size_t i = 0; i < len; i++) {
        stbrp_rect rect = rects[i];
        char32_t cp = char32_t(rect.id);

        math::uint2 point = { uint32_t(rect.x + 1), uint32_t(rect.y + 1) };
        math::uint2 size = { uint32_t(rect.w - 1), uint32_t(rect.h - 1) };

        bundle::GlyphInfo info = m_font.draw_glyph(cp, point, m_image, kWhiteFloat);

        // calculate uv coords
        float u0 = float(point.x) / float(m_image.size.width);
        float v0 = float(point.y) / float(m_image.size.height);

        float u1 = float(point.x + size.x) / float(m_image.size.width);
        float v1 = float(point.y + size.y) / float(m_image.size.height);

        GlyphInfo coords = { { u0, v0 }, { u1, v1 }, info.bearing, info.size };
        glyphs[cp] = coords;
    }

    stbrp_rect white_pixel = rects[len];
    math::uint2 px_point = { uint32_t(white_pixel.x), uint32_t(white_pixel.y) };
    math::uint2 px_size = { uint32_t(white_pixel.w), uint32_t(white_pixel.h) };

    // write white pixels into the image
    m_image.get_pixel_rgba(px_point.x, px_point.y)          = kWhite;
    m_image.get_pixel_rgba(px_point.x + 1, px_point.y)      = kWhite;
    m_image.get_pixel_rgba(px_point.x, px_point.y + 1)      = kWhite;
    m_image.get_pixel_rgba(px_point.x + 1, px_point.y + 1)  = kWhite;

    // get the uv coords of the middle of the 2x2 white pixel
    float u0 = float(px_point.x + 1) / float(m_image.size.width);
    float v0 = float(px_point.y + 1) / float(m_image.size.height);

    m_white_pixel_uv = { u0, v0 };
}
