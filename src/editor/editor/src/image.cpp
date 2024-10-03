#include "stdafx.hpp"

#include "archive/image.hpp"
#include "editor/editor.hpp"

#include "archive/io.hpp"

using namespace sm;
using namespace sm::ed;

world::IndexOf<world::Image> ed::loadImageInfo(world::World& world, const fs::path& path) {
    auto maybeImage = sm::openImage(path);
    if (!maybeImage.has_value()) {
        LOG_WARN(IoLog, "Failed to load image {}. {}", path.string(), maybeImage.error());
        return world::kInvalidIndex;
    }

    auto& image = maybeImage.value();

    size_t len = image.sizeInBytes();

    auto idx = world.add(world::Buffer {
        .name = path.filename().string(),
        .data = std::move(image.data),
    });

    return world.add(world::Image {
        .name = path.filename().string(),
        .source = world::BufferView {
            .source = idx,
            .offset = 0,
            .source_size = uint32(len),
        },
        .format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .size = image.size2d(),
        .mips = 1,
    });
}

void Editor::importImage(const fs::path& path) {
    world::IndexOf img = loadImageInfo(mContext.mWorld, path);
    if (img == world::kInvalidIndex) {
        alert_error(fmt::format("Failed to load image {}", path.string()));
        return;
    }

    mContext.upload([this, img] {
        mContext.upload_image(img);
    });
}

void Editor::importBlockCompressedImage(const fs::path& path) {
    DirectX::ScratchImage image;
    render::Result hr = DirectX::LoadFromDDSFile(path.c_str(), DirectX::DDS_FLAGS_FORCE_RGB, nullptr, image);

    if (hr.failed()) {
        alert_error(fmt::format("Failed to load DDS image {}", path.string()));
        return;
    }

    size_t len = image.GetPixelsSize();

    world::Buffer buffer = {
        .name = path.filename().string(),
        .data = std::vector<uint8>(image.GetPixels(), image.GetPixels() + len),
    };

    auto idx = mContext.mWorld.add(std::move(buffer));

    world::BufferView view = {
        .source = idx,
        .offset = 0,
        .source_size = uint32(len),
    };

    auto metadata = image.GetMetadata();

    world::Image info = {
        .name = path.filename().string(),
        .source = view,
        .format = metadata.format,
        .size = { uint(metadata.width), uint(metadata.height) },
        .mips = uint(metadata.mipLevels),
    };

    auto img = mContext.mWorld.add(std::move(info));

    mContext.upload([this, img] {
        mContext.upload_image(img);
    });
}
