#include "archive/bundle.hpp"

#include "archive/io.hpp"

#include "core/memory.h"
#include "core/memory.hpp"
#include "core/string.hpp"
#include "core/format.hpp" // IWYU pragma: keep

#include "core/units.hpp"
#include "logs/logs.hpp"

#include "artery-font/artery-font.h" // IWYU pragma: keep
#include "stb/stb_image.h"

#include "fs/fs.h"
#include "tar/tar.h"

using namespace sm;

Bundle::Bundle(fs_t *vfs)
    : mFileSystem(vfs)
{ }

Bundle::Bundle(io_t *stream, archive::BundleFormat type)
    : Bundle(fs_virtual("bundle", get_default_arena()))
{
    if (type != archive::BundleFormat::eTar) {
        logs::gAssets.warn("unsupported bundle format: {}", type);
        return;
    }

    if (tar_result_t err = tar_extract(*mFileSystem, stream); err.error != eTarOk) {
        logs::gAssets.error("failed to extract bundle: {}", tar_error_string(err.error));
    }
}

io_t *Bundle::get_file(sm::StringView dir, sm::StringView name) const {
    sm::String path = fmt::format("bundle/{}/{}", dir, name);
    io_t *file = fs_open(*mFileSystem, path.c_str(), eOsAccessRead);
    if (OsError err = io_error(file); err.failed()) {
        logs::gAssets.error("failed to open file: {}", err);
        return nullptr;
    }

    return file;
}

sm::Span<const uint8> Bundle::get_file_data(sm::StringView dir, sm::StringView name) const {
    IoHandle file = get_file(dir, name);
    if (!file.is_valid()) return {};

    // TODO: if we use anything other than tarfs then this becomes a use
    // after free
    size_t size = io_size(*file);
    const uint8 *data = (uint8*)io_map(*file, eOsProtectRead);

    logs::gAssets.info("loaded file {} ({} bytes)", name, sm::Memory{size});

    return { data, size };
}

ShaderIr Bundle::get_shader_bytecode(const char *name) const {
    return get_file_data("shaders", fmt::format("{}.cso", name));
}

TextureData Bundle::get_texture(const char *name) const {
    return get_file_data("textures", fmt::format("{}.dds", name));
}

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

static Format get_channel_format(int channels) {
    switch (channels) {
    case 1: return Format::eR8_BYTE;
    case 2: return Format::eRG8_BYTE;
    case 3: return Format::eRGB8_BYTE;
    case 4: return Format::eRGBA8_BYTE;
    default: return Format::eUnknown;
    }
}

// TODO: deduplicate this with the one in image.cpp
static ImageData convert_image(stbi_uc *pixels, int width, int height, int channels) {
    const ImageData image = {
        .format = ImageFormat::ePNG, // TODO: detect this properly
        .pxformat = get_channel_format(channels),
        .size = { int_cast<uint32_t>(width), int_cast<uint32_t>(height) },
        .data = sm::Vector<uint8>(pixels, pixels + int_cast<ptrdiff_t>(width * height * 4)),
    };

    return image;
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

font::FontInfo Bundle::get_font(const char *name) const {
    IoHandle file = get_file("fonts", fmt::format("{}.arfont", name));
    if (!file.is_valid()) return {};

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

        int width, height, channels;
        stbi_uc *pixels = stbi_load_from_memory(data.data(), (int)data.size(), &width, &height, &channels, 4);
        if (pixels == nullptr) {
            logs::gAssets.error("failed to load font texture: {}", stbi_failure_reason());
            return {};
        }

        logs::gAssets.info("| loaded image: {}x{}x{}", width, height, channels);
        images.emplace_back(convert_image(pixels, width, height, channels));
    }

    auto info = convert_info(font, name);
    info.images = std::move(images);

    return info;
}
