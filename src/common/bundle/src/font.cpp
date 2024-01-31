#include "bundle/bundle.hpp"

using namespace sm;
using namespace sm::bundle;

Font::Font(const AssetSink& log, BinaryData data, const char *name)
    : m_log(log)
    , m_name(name)
    , m_data(std::move(data))
{
    auto& ft = service::get_freetype();

    SM_ASSERT_FT2(FT_New_Memory_Face(ft.get_library(), m_data.data(), FT_Long(m_data.size()), 0, &m_face), "failed to load font {}", m_name);

    SM_ASSERT_FT2(FT_Select_Charmap(m_face, FT_ENCODING_UNICODE), "failed to select unicode charmap for font {}", m_name);
}

Font::~Font() {
    if (m_face == nullptr) return;

    m_log.info("unloading font {}", m_name);
    SM_ASSERT_FT2(FT_Done_Face(m_face), "failed to unload font {}", m_name);
    m_face = nullptr;
}
