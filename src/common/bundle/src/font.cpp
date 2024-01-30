#include "bundle/bundle.hpp"

#include "io/io.h"
#include "core/compiler.h"
#include "std/str.h"

using namespace sm;
using namespace sm::bundle;

Font::Font(const AssetSink& log, BinarySpan data, const char *name)
    : m_log(log)
{
    auto& ft = service::get_freetype();

    SM_ASSERT_FT2(FT_New_Memory_Face(ft.get_library(), data.data(), FT_Long(data.size_bytes()), 0, &m_face), "failed to load font {}", name);

    SM_ASSERT_FT2(FT_Select_Charmap(m_face, FT_ENCODING_UNICODE), "failed to select unicode charmap for font {}", name);
}

Font::~Font() {
    FT_Done_Face(m_face);
}
