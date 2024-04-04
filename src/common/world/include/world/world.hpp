#pragma once

#include "core/variant.hpp"

#include "world/mesh.hpp"

#include "archive/archive.hpp"
#include <dxgiformat.h>

namespace sm::world {
    using vtxindex = uint16; // NOLINT

    static constexpr uint16 kInvalidIndex = UINT16_MAX;

    template<typename T>
    class IndexOf {
        uint16 mIndex;

    public:
        IndexOf(uint16 index = kInvalidIndex)
            : mIndex(index)
        { }

        bool is_valid() const { return mIndex != kInvalidIndex; }

        operator uint16() const { return mIndex; }
        uint16 get() const { return mIndex; }
    };

    template<typename... T>
    using ChoiceOf = sm::Variant<IndexOf<T>...>;

    struct File {
        sm::String path;
    };

    struct Buffer {
        sm::String name;
        sm::Vector<uint8> data;
    };

    struct BufferView {
        ChoiceOf<File, Buffer> source;

        uint64 offset;
        uint32 file_size;
        uint32 buffer_size;
    };

    struct Image {
        sm::String name;
        BufferView source;
        DXGI_FORMAT format;
        math::uint2 size;
    };

    struct Texture {
        sm::String name;

        IndexOf<Image> image;
    };

    struct Material {
        sm::String name;

        float4 albedo;
        IndexOf<Texture> albedo_texture;
    };

    struct Object {
        uint32 vtx_count;
        uint32 idx_count;
        BufferView vertices;
        BufferView indices;
    };

    struct Model {
        sm::String name;
        sm::Variant<
            Object,
            Cube,
            Sphere,
            Cylinder,
            Plane,
            Wedge,
            Capsule,
            Diamond,
            GeoSphere
        > mesh;

        IndexOf<Material> material;
    };

    struct Node {
        sm::String name;

        Transform transform;

        sm::Vector<IndexOf<Node>> children;
        sm::Vector<IndexOf<Model>> models;
    };

    struct Camera {
        sm::String name;

        float3 position;
        float3 direction;

        float fov;
    };

    struct Scene {
        sm::String name;

        IndexOf<Node> root;
        IndexOf<Camera> camera;
    };

    using AnyIndex = ChoiceOf<Scene, Node, Camera, Model, File, Buffer, Material, Image, Texture>;

    struct World {
        sm::String name;
        IndexOf<Scene> active_scene;
        IndexOf<Material> default_material;

        sm::Vector<Scene> scenes;
        sm::Vector<Node> nodes;
        sm::Vector<Camera> cameras;
        sm::Vector<Model> models;
        sm::Vector<File> files;
        sm::Vector<Buffer> buffers;
        sm::Vector<Material> materials;
        sm::Vector<Image> images;
        sm::Vector<Texture> textures;

        template<typename T>
        IndexOf<T> add(T&& value) {
            auto& vec = get_vector<T>();
            uint16 index = int_cast<uint16>(vec.size());
            vec.push_back(std::forward<T>(value));
            return IndexOf<T>(index);
        }

        template<typename T>
        T *try_get(IndexOf<T> index) {
            if (index.is_valid())
                return &get<T>(index);

            return nullptr;
        }

        template<typename T>
        T &get(IndexOf<T> index) { return get_vector<T>()[index]; }

        auto visit(AnyIndex index, auto&& fn) {
            return std::visit([&](auto index) { return fn(get(index)); }, index);
        }

        template<typename T>
        sm::Vector<T>& get_vector();

        template<> sm::Vector<Node>& get_vector() { return nodes; }
        template<> sm::Vector<Camera>& get_vector() { return cameras; }
        template<> sm::Vector<Model>& get_vector() { return models; }
        template<> sm::Vector<File>& get_vector() { return files; }
        template<> sm::Vector<Buffer>& get_vector() { return buffers; }
        template<> sm::Vector<Material>& get_vector() { return materials; }
        template<> sm::Vector<Image>& get_vector() { return images; }
        template<> sm::Vector<Texture>& get_vector() { return textures; }
    };

    World empty_world(sm::String name);
    World default_world(sm::String name);

    bool load_world(World& info, Archive& archive);
    void save_world(Archive& archive, const World& world);
}
