#include "stdafx.hpp"

#include "archive/image.hpp"
#include "editor/editor.hpp"

using namespace sm;
using namespace sm::ed;

world::IndexOf<world::Image> ed::loadImageInfo(world::World& world, const fs::path& path) {
    ImageData image = sm::open_image(path);
    if (!image.is_valid()) {
        return world::kInvalidIndex;
    }

    size_t len = image.data.size();

    world::Buffer buffer = {
        .name = path.filename().string(),
        .data = std::move(image.data),
    };

    auto idx = world.add(std::move(buffer));

    world::BufferView view = {
        .source = idx,
        .offset = 0,
        .source_size = uint32(len),
    };

    world::Image info = {
        .name = path.filename().string(),
        .source = view,
        .format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .size = image.size,
        .mips = 1,
    };

    return world.add(std::move(info));
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
