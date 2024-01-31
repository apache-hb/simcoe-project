#include "bundle/bundle.hpp"

using namespace sm;
using namespace sm::bundle;

static void blit_glyph(const AssetSink& logs, Image& image, char32_t codepoint, FT_String *name, FT_Bitmap *bitmap, FT_UInt x, FT_UInt y, const math::float4& colour) {
    auto write_pixel = [&](size_t x, size_t y, uint8_t alpha, math::float4 col) {
        size_t index = (y * image.size.width + x) * 4;
        if (index + 3 > image.data.size()) return;

        image.data[index + 0] = uint8_t(col.r * 255.f);
        image.data[index + 1] = uint8_t(col.g * 255.f);
        image.data[index + 2] = uint8_t(col.b * 255.f);
        image.data[index + 3] = uint8_t(alpha);
    };

    if (bitmap->pixel_mode == FT_PIXEL_MODE_GRAY) {
        for (FT_UInt row = 0; row < bitmap->rows; row++) {
            for (FT_UInt col = 0; col < bitmap->width; col++) {
                FT_UInt index = row * bitmap->pitch + col;
                FT_Byte alpha = bitmap->buffer[index];

                write_pixel(x + col, y + row, uint8_t(alpha), colour);
            }
        }
    } else if (bitmap->pixel_mode == FT_PIXEL_MODE_MONO) {
        for (FT_UInt row = 0; row < bitmap->rows; row++) {
            for (FT_UInt col = 0; col < bitmap->width; col++) {
                FT_UInt index = row * bitmap->pitch + col / 8;
                FT_Byte alpha = (bitmap->buffer[index] & (0x80 >> (col % 8))) ? 255 : 0;

                write_pixel(x + col, y + row, uint8_t(alpha), colour);
            }
        }
    } else {
        // fill the glyph with magenta
        for (FT_UInt row = 0; row < bitmap->rows; row++) {
            for (FT_UInt col = 0; col < bitmap->width; col++) {
                write_pixel(x + col, y + row, uint8_t(255), math::float4(1.f, 0.f, 1.f, 1.f));
            }
        }

        logs.warn("unsupported pixel mode `{}` `{}` (mode={})", char(codepoint), name, bitmap->pixel_mode);
    }
}

Font::Font(const AssetSink& log, BinaryData data, const char *name)
    : m_log(log)
    , m_name(name)
    , m_data(std::move(data))
{
    auto& ft = service::get_freetype();

    SM_ASSERT_FT2(FT_New_Memory_Face(ft.get_library(), m_data.data(), FT_Long(m_data.size()), 0, &m_face), "failed to load font {}", m_name);

    SM_ASSERT_FT2(FT_Select_Charmap(m_face, FT_ENCODING_UNICODE), "failed to select unicode charmap for font {}", m_name);

    m_log.info("loaded font {}", m_name);
    m_log.info("| family: {}", m_face->family_name);
    m_log.info("| style: {}", m_face->style_name);
    m_log.info("| glyphs: {}", m_face->num_glyphs);

    SM_ASSERT_FT2(FT_Set_Char_Size(m_face, 0, 16 * 64, 300, 300), "failed to set font size for font {}", m_name);

    FT_Size_Metrics metrics = m_face->size->metrics;
    m_info.ascender = float(metrics.ascender) / 64.f;
    m_info.descender = float(metrics.descender) / 64.f;
}

Font::~Font() {
    if (m_face == nullptr) return;

    m_log.info("unloading font {}", m_name);
    SM_ASSERT_FT2(FT_Done_Face(m_face), "failed to unload font {}", m_name);
    m_face = nullptr;
}

math::uint2 Font::get_glyph_size(char32_t codepoint) const {
    SM_ASSERT_FT2(FT_Load_Char(m_face, codepoint, FT_LOAD_DEFAULT), "failed to load glyph for codepoint {}", int32_t(codepoint));

    return math::uint2(m_face->glyph->bitmap.width, m_face->glyph->bitmap.rows);
}

GlyphInfo Font::draw_glyph(char32_t codepoint, math::uint2 start, Image& image, const math::float4& color) const {
    FT_Set_Transform(m_face, nullptr, nullptr);

    SM_ASSERT_FT2(FT_Load_Char(m_face, codepoint, FT_LOAD_RENDER), "failed to load glyph for codepoint {}", int32_t(codepoint));

    FT_Bitmap *bitmap = &m_face->glyph->bitmap;
    blit_glyph(m_log, image, codepoint, m_face->family_name, bitmap, start.x, start.y, color);

    GlyphInfo info = {
        .size = { int(bitmap->width), int(bitmap->rows) },
        .bearing = { m_face->glyph->bitmap_left, m_face->glyph->bitmap_top }
    };

    return info;
}
