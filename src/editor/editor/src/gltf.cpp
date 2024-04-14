#include "stdafx.hpp"

#include "editor/editor.hpp"

using namespace sm;
using namespace sm::ed;

namespace fg = fastgltf;

static constexpr fg::Extensions kExt = fg::Extensions::None;

static constexpr fg::Options kOptions
    = fg::Options::DontRequireValidAssetMember
    | fg::Options::DecomposeNodeMatrices
    | fg::Options::GenerateMeshIndices
    | fg::Options::LoadExternalBuffers
    // | fg::Options::LoadExternalImages
    ;


void Editor::importGltf(const fs::path& path) {
    if (!fs::exists(path)) {
        alert_error(fmt::format("File not found: {}", path.string()));
        return;
    }

    fg::Parser parser{kExt};
    fg::GltfDataBuffer buffer;

    buffer.loadFromFile(path.string());

    auto parent = path.parent_path();

    auto asset = parser.loadGltf(&buffer, parent, kOptions, fg::Category::OnlyRenderable);
    if (asset.error() != fg::Error::None) {
        alert_error(fmt::format("Failed to load glTF: {} (parent path: {})", fg::getErrorMessage(asset.error()), parent.string()));
        return;
    }

    auto gltfLoadImage = [&](uint index, const fg::Image& image) {
        std::visit(fg::visitor {
            [&](const auto& arg) {
                logs::gAssets.warn("Unknown image load type (name: {})", image.name);
            },
            [&](const fg::sources::URI& uri) {
                logs::gAssets.info("Loading image from URI: {}", uri.uri.fspath().string());
                world::IndexOf img = loadImageInfo(mContext.mWorld, parent / uri.uri.fspath());
                if (img != world::kInvalidIndex) {
                    mContext.upload_image(img);
                } else {
                    logs::gAssets.error("Failed to load image: {}", image.name);
                }
            },
            [&](const fg::sources::Array& array) {
                logs::gAssets.info("Loading image from array: {}", image.name);
            },
            [&](const fg::sources::BufferView& view) {
                logs::gAssets.info("Loading image from buffer view: {}", image.name);
            }
        }, image.data);

        return world::kInvalidIndex;
    };

    mContext.upload([&] {
        const fg::Asset& val = asset.get();
        // auto root = get_best_node();

        // Builder builder { mContext, val };

        for (size_t i = 0; i < val.images.size(); i++) {
            gltfLoadImage(i, val.images[i]);
        }
    });

    logs::gAssets.info("Loaded glTF: {}", path.string());
}
