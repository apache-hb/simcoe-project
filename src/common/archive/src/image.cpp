#include "stdafx.hpp"

#include "archive/image.hpp"

#include "archive/io.hpp"
#include "core/units.hpp"

#include "logs/logs.hpp"

using namespace sm;

static constexpr Format getChannelFormat(int channels) {
    switch (channels) {
    case 1: return Format::eR8_BYTE;
    case 2: return Format::eRG8_BYTE;
    case 3: return Format::eRGB8_BYTE;
    case 4: return Format::eRGBA8_BYTE;
    default: return Format::eUnknown;
    }
}

std::expected<ImageData, std::string> sm::loadImage(std::span<const byte> data) {
    int width, height;
    stbi_uc *pixels = stbi_load_from_memory((const stbi_uc*)data.data(), int_cast<int>(data.size()), &width, &height, nullptr, 4);
    if (pixels == nullptr)
        return std::unexpected(fmt::format("stbi failed to load image {}", stbi_failure_reason()));

    ImageData image = {
        .pxformat = getChannelFormat(4),
        .size = { int_cast<uint32_t>(width), int_cast<uint32_t>(height) },
        // TODO: this is a full copy, maybe theres a way to use a single buffer
        .data = sm::Vector<uint8>(pixels, pixels + int_cast<ptrdiff_t>(width * height * 4)),
    };

    stbi_image_free(pixels);

    return image;
}

std::expected<ImageData, std::string> sm::openImage(const fs::path& path) {
    if (!fs::exists(path))
        return std::unexpected(fmt::format("Image file `{}` does not exist", path));

    auto file = Io::file(path.string().c_str(), eOsAccessRead);
    if (!file.isValid())
        return std::unexpected(fmt::format("Failed to open image file `{}`: {}", path, file.error()));

    auto size = file.size();
    if (size == 0)
        return std::unexpected(fmt::format("Image file `{}` is empty", path));

    void *data = io_map(*file, eOsProtectRead);
    if (data == nullptr)
        return std::unexpected(fmt::format("Failed to map image file `{}`: {}", path, file.error()));

    return loadImage({ reinterpret_cast<const byte*>(data), size });
}
