#include "stdafx.hpp"

#include "render/render.hpp"

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

class Builder {
    sm::HashMap<uint, uint16> mImageLookup;
    sm::HashMap<uint, uint16> mMaterialLookup;
    sm::HashMap<uint, uint16> mMeshLookup;
    sm::HashMap<uint, uint16> mCameraLookup;
    sm::HashMap<uint, uint16> mNodeLookup;

    render::Context& mContext;
    fg::Asset& mAsset;

public:
    Builder(render::Context& ctx, fg::Asset& asset)
        : mContext(ctx)
        , mAsset(asset)
    { }

    uint16 gltf_load_image(uint index, fg::Image& image) {
        return std::visit(fg::visitor {
            [&](auto& arg) {
                gSink.warn("Unknown image load type (name: {})", image.name);
                return false;
            },
            [&](fg::sources::URI& uri) {
                mImageLookup[index] = mContext.load_texture(uri.uri.fspath());
                return true;
            },
            [&](fg::sources::Array& arr) {
                auto data = sm::load_image(arr.bytes);
                if (data.size == 0u) {
                    gSink.error("Failed to load image: {}", image.name);
                    return false;
                }

                mImageLookup[index] = mContext.load_texture(data);
                return true;
            },
            [&](fg::sources::BufferView& view) {
                auto& bv = mAsset.bufferViews[view.bufferViewIndex];
                auto& buffer = mAsset.buffers[bv.bufferIndex];

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

                        mImageLookup[index] = mContext.load_texture(data);
                        return true;
                    }
                }, buffer.data);
            }
        }, image.data);
    }

    bool gltf_load_material(uint index, fg::Material& material) {
        return true;
    }

    bool gltf_load_mesh(uint index, fg::Mesh& mesh) {
        for (auto it = mesh.primitives.begin(); it != mesh.primitives.end(); ++it) {
            auto *position = it->findAttribute("POSITION");
            if (position == it->attributes.end()) {
                gSink.error("Mesh primitive {} has no position attribute", mesh.name);
                return false;
            }

            sm::Vector<world::Vertex> vertices;
            sm::Vector<uint16> indices;
        }

        return true;
    }

    bool gltf_load_camera(uint index, fg::Camera& camera) {
        return true;
    }

    bool gltf_load_node(uint index, fg::Node& node) {
        if (!std::holds_alternative<fg::TRS>(node.transform)) {
            gSink.error("Node {} has unknown transform type", node.name);
            return false;
        }
        fg::TRS trs = std::get<fg::TRS>(node.transform);
        math::quatf rotation = { trs.rotation[0], trs.rotation[1], trs.rotation[2], trs.rotation[3] };
        world::Transform transform = {
            .position = { trs.translation[0], trs.translation[1], trs.translation[2] },
            .rotation = rotation.to_euler(),
            .scale = { trs.scale[0], trs.scale[1], trs.scale[2] },
        };

        world::NodeInfo info = {
            .name = sm::String{node.name.c_str()},
            .transform = transform,
        };

        mNodeLookup[index] = mContext.mWorld.info.add_node(info);
        return true;
    }

    void gltf_setup_node(uint16 index, fg::Node& node) {
        uint16 self = mNodeLookup[index];
        auto& info = mContext.mWorld.info.nodes[self];

        for (size_t i = 0; i < node.children.size(); i++) {
            uint16 child = mNodeLookup[node.children[i]];
            auto& child_info = mContext.mWorld.info.nodes[child];

            child_info.parent = self;
            info.children.push_back(child);
        }

        if (!node.meshIndex.has_value())
            return;

        uint16 mesh = mMeshLookup[node.meshIndex.value()];
        mContext.mWorld.info.add_node_object(self, mesh);
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

    auto parent = path.parent_path();
    auto asset = parser.loadGltf(&data, parent, kOptions, fg::Category::OnlyRenderable);
    if (asset.error() != fg::Error::None) {
        gSink.error("Failed to parse GLTF file: {} (parent path: {})", fg::getErrorMessage(asset.error()), parent);
        return false;
    }

    auto& val = asset.get();

    Builder builder{*this, val};

    gSink.info("Loading GLTF: {}", path);

    for (size_t i = 0; i < val.images.size(); ++i) {
        if (!builder.gltf_load_image((uint)i, val.images[i]))
            return false;
    }

    for (size_t i = 0; i < val.materials.size(); ++i) {
        if (!builder.gltf_load_material(i, val.materials[i]))
            return false;
    }

    for (size_t i = 0; i < val.meshes.size(); ++i) {
        if (!builder.gltf_load_mesh(i, val.meshes[i]))
            return false;
    }

    for (size_t i = 0; i < val.cameras.size(); ++i) {
        if (!builder.gltf_load_camera(i, val.cameras[i]))
            return false;
    }

    for (size_t i = 0; i < val.nodes.size(); i++) {
        if (!builder.gltf_load_node(i, val.nodes[i]))
            return false;
    }

    for (size_t i = 0; i < val.nodes.size(); i++) {
        builder.gltf_setup_node(i, val.nodes[i]);
    }

    return true;
}
