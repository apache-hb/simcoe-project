#include "archive/image.hpp"

#include "archive/io.hpp"
#include "core/units.hpp"

#include "logs/logs.hpp"

#include "stb/stb_image.h"

#include "fmt/std.h" // IWYU pragma: keep

using namespace sm;

static auto gSink = logs::get_sink(logs::Category::eAssets);

static constexpr Format get_channel_format(int channels) {
    switch (channels) {
    case 1: return Format::eR8_BYTE;
    case 2: return Format::eRG8_BYTE;
    case 3: return Format::eRGB8_BYTE;
    case 4: return Format::eRGBA8_BYTE;
    default: return Format::eUnknown;
    }
}

static constexpr ImageFormat get_image_format(int type) {
    switch (type) {
    case STBI_png: return ImageFormat::ePNG;
    case STBI_jpeg: return ImageFormat::eJPG;
    case STBI_bmp: return ImageFormat::eBMP;
    default: return ImageFormat::eUnknown;
    }
}

ImageData sm::load_image(sm::Span<const uint8> data) {
    int width, height, channels;
    stbi_uc *pixels = stbi_load_from_memory(data.data(), int_cast<int>(data.size()), &width, &height, &channels, 4);
    if (!pixels) {
        gSink.error("Failed to load image: {}", stbi_failure_reason());
        return {};
    }

    int type = stbi_test_from_memory(data.data(), int_cast<int>(data.size()));
    ImageFormat imgtype = get_image_format(type);

    const ImageData image = {
        .format = imgtype,
        .pxformat = get_channel_format(channels),
        .size = { int_cast<uint32_t>(width), int_cast<uint32_t>(height) },
        .data = sm::Vector<uint8>(pixels, pixels + int_cast<ptrdiff_t>(width * height * 4)),
    };

    stbi_image_free(pixels);

    return image;
}

ImageData sm::open_image(const fs::path& path) {
    if (!fs::exists(path)) {
        gSink.error("Image file `{}` does not exist", path);
        return {};
    }

    auto file = Io::file(path.string().c_str(), eOsAccessRead);
    if (!file.is_valid()) {
        gSink.error("Failed to open image file `{}`: {}", path, file.error());
        return {};
    }

    auto size = file.size();
    if (size == 0) {
        gSink.error("Image file `{}` is empty", path);
        return {};
    }

    void *data = io_map(*file, eOsProtectRead);
    if (!data) {
        gSink.error("Failed to map image file `{}`: {}", path, file.error());
        return {};
    }

    return load_image({ reinterpret_cast<const uint8 *>(data), size });
}
