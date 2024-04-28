#include "stdafx.hpp"

#include "archive/bundle.hpp"

#include "core/string.hpp"
#include "core/format.hpp" // IWYU pragma: keep

#include "logs/logs.hpp"

using namespace sm;

Bundle::Bundle(IFileSystem *fs)
    : mFileSystem(fs)
{ }

sm::Span<const uint8> Bundle::getFileData(sm::StringView dir, sm::StringView name) {
    sm::View data = mFileSystem->readFileData(fmt::format("bundle/{}/{}", dir, name));
    if (data.empty()) {
        logs::gAssets.error("failed to read file: {}/{}", dir, name);
        return {};
    }

    return data;
}

ShaderIr Bundle::get_shader_bytecode(const char *name) {
    return getFileData("shaders", fmt::format("{}.cso", name));
}

TextureData Bundle::get_texture(const char *name) {
    return getFileData("textures", fmt::format("{}.dds", name));
}

#if 0
static int wrap_io_read(void *dst, int length, void *user) {
    if (dst == nullptr || length < 0) return 0;

    io_t *io = (io_t*)user;
    return int_cast<int>(io_read(io, dst, int_cast<size_t>(length)));
}

// i have some choice words for artery-fonts api design
template<typename T>
struct ArteryVector : sm::Vector<T> {
    using Super = sm::Vector<T>;
    using Super::Super;

    operator T*() { return Super::data(); }
};

using ArteryByteArray = ArteryVector<unsigned char>;
namespace af = artery_font;

namespace artery {
    using real = float; // NOLINT

    using FontData = af::ArteryFont<real, ArteryVector, ArteryByteArray, sm::String>;
    using Variant = typename FontData::Variant;
    using Image = typename FontData::Image;
    using Appendix = typename FontData::Appendix;
    using KerningPair = af::KernPair<real>;
    using Glyph = af::Glyph<real>;
}

static constexpr font::KerningPair convert_kerning_pair(const artery::KerningPair& pair) {
    const font::KerningPair result = {
        .first = pair.codepoint1,
        .second = pair.codepoint2,
        .advance = {
            .h = pair.advance.h,
            .v = pair.advance.v,
        },
    };

    return result;
}

static constexpr font::Glyph convert_glyph(const artery::Glyph& glyph) {
    const auto& [cp, _0, plane, image, advance] = glyph;
    const font::Glyph result = {
        .codepoint = cp,
        .image = sm::int_cast<uint16>(glyph.image),
        .plane = { plane.l, plane.t, plane.r, plane.b, },
        .texbounds = { image.l, image.t, image.r, image.b, },
        .advance = {
            .h = advance.h,
            .v = advance.v,
        },
    };

    return result;
}

static constexpr font::Family convert_family(const artery::Variant& variant) {
    const auto& m = variant.metrics;
    font::Family result = {
        .name = variant.name,
        .metrics = {
            .size = m.fontSize,
            .range = m.distanceRange,
            .em_size = m.emSize,
            .ascender = m.ascender,
            .descender = m.descender,
            .line_height = m.lineHeight,
            .underline_y = m.underlineY,
            .underline_thickness = m.underlineThickness,
        }
    };

    for (const auto& glyph : variant.glyphs)
        result.glyphs.emplace_back(convert_glyph(glyph));

    for (const auto& kerning : variant.kernPairs)
        result.kerning.emplace_back(convert_kerning_pair(kerning));

    return result;
}

static constexpr font::FontInfo convert_info(const artery::FontData& font, const char *name) {
    font::FontInfo result = {
        .name = name,
    };

    for (const auto& variant : font.variants)
        result.families.emplace_back(convert_family(variant));

    return result;
}

font::FontInfo Bundle::get_font(const char *name) {
    sm::View data = getFileData("fonts", fmt::format("{}.arfont", name));
    if (data.empty()) return {};

    artery::FontData font;
    bool result = af::decode<wrap_io_read>(font, (void*)file.get());
    if (!result) {
        logs::gAssets.error("failed to decode font: {}", name);
        return {};
    }

    logs::gAssets.info("font {} (images {}, variants {})", name, font.images.size(), font.variants.size());

    if (font.metadataFormat != artery_font::METADATA_NONE) {
        logs::gAssets.info("| metadata: {}", font.metadata);
    }

    for (const auto& appendix : font.appendices) {
        logs::gAssets.info("| appendix: {}", appendix.metadata);
    }

    if (font.images.empty()) {
        logs::gAssets.error("font has no images, cannot load data");
        return {};
    }

    sm::Vector<ImageData> images;

    for (auto& image : font.images) {
        const auto& data = image.data;
        logs::gAssets.info("| image: {}x{}x{}", image.width, image.height, image.channels);

        images.emplace_back(load_image({ data.data(), data.size() }));
    }

    auto info = convert_info(font, name);
    info.images = std::move(images);

    return info;
}
#endif
