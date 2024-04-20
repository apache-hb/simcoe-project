#include "stdafx.hpp"

#include "archive/image.hpp"

#include "archive/io.hpp"
#include "core/units.hpp"

#include "logs/logs.hpp"

using namespace sm;

static constexpr Format get_channel_format(int channels) {
    switch (channels) {
    case 1: return Format::eR8_BYTE;
    case 2: return Format::eRG8_BYTE;
    case 3: return Format::eRGB8_BYTE;
    case 4: return Format::eRGBA8_BYTE;
    default: return Format::eUnknown;
    }
}

ImageData sm::load_image(sm::Span<const uint8> data) {
    int width, height;
    stbi_uc *pixels = stbi_load_from_memory(data.data(), int_cast<int>(data.size()), &width, &height, nullptr, 4);
    if (!pixels) {
        logs::gAssets.warn("Failed to parse image data: {}", stbi_failure_reason());
        return {};
    }

    const ImageData image = {
        .pxformat = get_channel_format(4),
        .size = { int_cast<uint32_t>(width), int_cast<uint32_t>(height) },
        .data = sm::Vector<uint8>(pixels, pixels + int_cast<ptrdiff_t>(width * height * 4)),
    };

    stbi_image_free(pixels);

    return image;
}

ImageData sm::open_image(const fs::path& path) {
    if (!fs::exists(path)) {
        logs::gAssets.error("Image file `{}` does not exist", path);
        return {};
    }

    auto file = Io::file(path.string().c_str(), eOsAccessRead);
    if (!file.is_valid()) {
        logs::gAssets.error("Failed to open image file `{}`: {}", path, file.error());
        return {};
    }

    auto size = file.size();
    if (size == 0) {
        logs::gAssets.error("Image file `{}` is empty", path);
        return {};
    }

    void *data = io_map(*file, eOsProtectRead);
    if (!data) {
        logs::gAssets.error("Failed to map image file `{}`: {}", path, file.error());
        return {};
    }

    return load_image({ reinterpret_cast<const uint8 *>(data), size });
}
