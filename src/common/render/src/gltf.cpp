#include "render/render.hpp"

#include "logs/logs.hpp"

#include "stdafx.hpp"

using namespace sm;
using namespace sm::render;

namespace fg = fastgltf;

static auto gSink = logs::get_sink(logs::Category::eAssets);

static constexpr auto kExt = fg::Extensions::None;
static constexpr auto kOptions
    = fg::Options::DontRequireValidAssetMember
    | fg::Options::DecomposeNodeMatrices
    | fg::Options::GenerateMeshIndices

    // TODO: use dstorage to load external buffers and images
    | fg::Options::LoadExternalBuffers
    | fg::Options::LoadExternalImages;

struct Builder {
    render::Context& ctx;
    fg::Asset& asset;

    uint16 gltf_load_image(fg::Image& image) {
        return std::visit(fg::visitor {
            [&](auto& arg) {
                gSink.warn("Unknown image load type (name: {})", image.name);
                return false;
            },
            [&](fg::sources::URI& uri) {
                ctx.load_texture(uri.uri.fspath());
                return true;
            },
            [&](fg::sources::Array& arr) {
                auto data = sm::load_image(arr.bytes);
                if (data.size == 0u) {
                    gSink.error("Failed to load image: {}", image.name);
                    return false;
                }

                ctx.load_texture(data);
                return true;
            },
            [&](fg::sources::BufferView& view) {
                auto& bv = asset.bufferViews[view.bufferViewIndex];
                auto& buffer = asset.buffers[bv.bufferIndex];

                return std::visit(fg::visitor {
                    [&](auto& arg) {
                        gSink.warn("Unknown buffer type (name: {}, buffer: {})", image.name, buffer.name);
                        return false;
                    },
                    [&](fg::sources::Array& arr) {
                        auto span = sm::Span<const uint8>{arr.bytes.data() + bv.byteOffset, bv.byteLength};
                        auto data = sm::load_image(span);
                        if (data.size == 0u) {
                            gSink.error("Failed to load image: {}", image.name);
                            return false;
                        }

                        ctx.load_texture(data);
                        return true;
                    }
                }, buffer.data);
            }
        }, image.data);
    }
};

bool Context::load_gltf(const fs::path& path) {
    if (!fs::exists(path)) {
        gSink.error("GLTF file does not exist: {}", path);
        return false;
    }

    fg::Parser parser{kExt};
    fg::GltfDataBuffer data;

    data.loadFromFile(path);

    auto asset = parser.loadGltf(&data, path.parent_path(), kOptions, fg::Category::OnlyRenderable);
    if (asset.error() != fg::Error::None) {
        gSink.error("Failed to parse GLTF file: {}", fg::getErrorMessage(asset.error()));
        return false;
    }

    auto& val = asset.get();

    Builder builder{*this, val};

    gSink.info("Loading GLTF: {}", path);

    for (auto& image : val.images) {
        if (!builder.gltf_load_image(image))
            return false;
    }

    return true;
}
