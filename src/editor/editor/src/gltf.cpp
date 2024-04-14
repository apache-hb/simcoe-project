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

template<>
struct fg::ElementTraits<math::float2> : fg::ElementTraitsBase<math::float2, fg::AccessorType::Vec2, float> {};

template<>
struct fg::ElementTraits<math::float3> : fg::ElementTraitsBase<math::float3, fg::AccessorType::Vec3, float> {};

template<>
struct fg::ElementTraits<math::float4> : fg::ElementTraitsBase<math::float4, fg::AccessorType::Vec4, float> {};

void Editor::importGltf(const fs::path& path) {
    if (!fs::exists(path)) {
        alert_error(fmt::format("File not found: {}", path.string()));
        return;
    }

    fg::Parser parser{kExt};
    fg::GltfDataBuffer buffer;

    buffer.loadFromFile(path.string());

    auto parent = path.parent_path();

    auto result = parser.loadGltf(&buffer, parent, kOptions, fg::Category::OnlyRenderable);
    if (result.error() != fg::Error::None) {
        alert_error(fmt::format("Failed to load glTF: {} (parent path: {})", fg::getErrorMessage(result.error()), parent.string()));
        return;
    }

    const fg::Asset& asset = result.get();

    sm::HashMap<uint, world::IndexOf<world::Image>> imageMap;

    // map of glTF model index to world model index
    sm::HashMap<uint, sm::Vector<world::IndexOf<world::Model>>> modelMap;

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

                    imageMap.emplace(index, img);
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
    };

    auto gltfLoadPrimitive = [&](const fg::Primitive& primitive, sm::String name)
        -> world::IndexOf<world::Model>
    {
        auto *posAttr = primitive.findAttribute("POSITION");
        auto *uvAttr = primitive.findAttribute("TEXCOORD_0");
        auto *normalAttr = primitive.findAttribute("NORMAL");
        auto *tangentAttr = primitive.findAttribute("TANGENT");

        if (posAttr == primitive.attributes.end()) {
            logs::gAssets.error("Primitive has no position attribute");
            return world::kInvalidIndex;
        }

        if (!primitive.indicesAccessor.has_value()) {
            logs::gAssets.error("Primitive has no indices accessor");
            return world::kInvalidIndex;
        }

        auto& indexAccess = asset.accessors[primitive.indicesAccessor.value()];
        if (!indexAccess.bufferViewIndex.has_value()) {
            logs::gAssets.error("Indices accessor has no buffer view");
            return world::kInvalidIndex;
        }


        auto& posAccess = asset.accessors[posAttr->second];
        if (!posAccess.bufferViewIndex.has_value()) {
            logs::gAssets.error("Position accessor has no buffer view");
            return world::kInvalidIndex;
        }

        // place all the primitive data in a single buffer
        // indices first, then vertices
        sm::Vector<uint8> data(indexAccess.count * sizeof(uint16) + posAccess.count * sizeof(world::Vertex));

        sm::Span<uint16> indices(reinterpret_cast<uint16*>(data.data()), indexAccess.count);
        sm::Span<world::Vertex> vertices(reinterpret_cast<world::Vertex*>(data.data() + indices.size_bytes()), posAccess.count);

        fg::iterateAccessorWithIndex<uint16>(asset, indexAccess, [&](uint16 value, size_t index) {
            indices[index] = value;
        });

        fg::iterateAccessorWithIndex<math::float3>(asset, posAccess, [&](const math::float3& value, size_t index) {
            vertices[index].position = value;
        });

        if (uvAttr != primitive.attributes.end()) {
            auto& uvAccess = asset.accessors[uvAttr->second];
            if (uvAccess.bufferViewIndex.has_value()) {
                fg::iterateAccessorWithIndex<math::float2>(asset, uvAccess, [&](const math::float2& value, size_t index) {
                    vertices[index].uv = value;
                });
            }
        }

        if (normalAttr != primitive.attributes.end()) {
            auto& normalAccess = asset.accessors[normalAttr->second];
            if (normalAccess.bufferViewIndex.has_value()) {
                fg::iterateAccessorWithIndex<math::float3>(asset, normalAccess, [&](const math::float3& value, size_t index) {
                    vertices[index].normal = value;
                });
            }
        }

        if (tangentAttr != primitive.attributes.end()) {
            auto& tangentAccess = asset.accessors[tangentAttr->second];
            if (tangentAccess.bufferViewIndex.has_value()) {
                fg::iterateAccessorWithIndex<math::float4>(asset, tangentAccess, [&](const math::float4& value, size_t index) {
                    vertices[index].tangent = value.xyz() * value.w;
                });
            }
        }

        uint32 vtxCount = static_cast<uint32>(vertices.size());
        uint32 idxCount = static_cast<uint32>(indices.size());

        world::IndexOf buffer = mContext.mWorld.add(world::Buffer {
            .name = fmt::format("{}.buffer", name),
            .data = std::move(data),
        });

        world::Object obj = {
            .vtx_count = vtxCount,
            .idx_count = idxCount,

            .vertices = world::BufferView::buffer(buffer, idxCount * sizeof(uint16), vtxCount * sizeof(world::Vertex)),
            .indices = world::BufferView::buffer(buffer, 0, idxCount * sizeof(uint16)),
        };

        world::IndexOf model = mContext.mWorld.add(world::Model {
            .name = std::move(name),
            .mesh = obj,
            .material = mContext.mWorld.default_material,
        });

        mContext.upload_model(model);

        return model;
    };

    auto gltfLoadMesh = [&](uint index, const fg::Mesh& mesh) {
        sm::String name = [&] -> sm::String {
            if (mesh.name.empty()) {
                return fmt::format("mesh{}", index);
            } else {
                return sm::String{mesh.name};
            }
        }();

        logs::gAssets.info("Loading mesh: {}", name);
        sm::Vector<world::IndexOf<world::Model>> models;
        for (size_t i = 0; i < mesh.primitives.size(); i++) {
            world::IndexOf index = gltfLoadPrimitive(mesh.primitives[i], fmt::format("{}.{}", name, i));
            if (index != world::kInvalidIndex) {
                models.push_back(index);
            }
        }

        modelMap.emplace(index, std::move(models));
    };

    mContext.upload([&] {
        // auto root = get_best_node();

        // Builder builder { mContext, val };

        for (size_t i = 0; i < asset.meshes.size(); i++) {
            gltfLoadMesh(i, asset.meshes[i]);
        }

        for (size_t i = 0; i < asset.images.size(); i++) {
            gltfLoadImage(i, asset.images[i]);
        }
    });

    logs::gAssets.info("Loaded glTF: {}", path.string());
}
