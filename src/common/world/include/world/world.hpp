#pragma once

#include "core/variant.hpp"

#include "world/mesh.hpp"

#include "archive/archive.hpp"

#include <directx/dxgiformat.h>
#include <directx/d3d12.h>

namespace sm::world {
    static constexpr uint16 kInvalidIndex = UINT16_MAX;

    enum IndexType {
        eNone,

        eScene,
        eNode,
        eCamera,
        eModel,
        eFile,
        eLight,
        eBuffer,
        eMaterial,
        eImage,

        eCount
    };

    template<typename T>
    concept IsWorldObject = requires {
        { T::kType } -> std::convertible_to<IndexType>;
    };

    template<IsWorldObject T>
    class IndexOf {
        uint16 mIndex;

    public:
        IndexOf(uint16 index = kInvalidIndex)
            : mIndex(index)
        { }

        template<typename O> requires (!std::is_same_v<T, O>)
        IndexOf(IndexOf<O>) = delete;

        bool is_valid() const { return mIndex != kInvalidIndex; }

        operator uint16() const { return mIndex; }
        uint16 get() const { return mIndex; }

        static IndexType type() { return T::kType; }
    };

    template<IsWorldObject... T>
    using ChoiceOf = sm::Variant<IndexOf<T>...>;

    template<IsWorldObject... T>
    using OptionOf = sm::Variant<IndexOf<T>..., std::monostate>;

    template<IsWorldObject T, typename... A>
    IndexOf<T> get(ChoiceOf<A...> index) {
        if (auto *value = std::get_if<IndexOf<T>>(&index))
            return *value;

        return kInvalidIndex;
    }

    struct File {
        static constexpr IndexType kType = eFile;
        static constexpr sm::StringView kName = "File";

        sm::String path;
    };

    struct Buffer {
        static constexpr IndexType kType = eBuffer;
        static constexpr sm::StringView kName = "Buffer";

        sm::String name;
        sm::Vector<uint8> data;
    };

    struct BufferView {
        ChoiceOf<File, Buffer> source;

        uint64 offset;
        uint32 source_size;
        // uint32 buffer_size;
    };

    struct Image {
        static constexpr IndexType kType = eImage;
        static constexpr sm::StringView kName = "Image";

        sm::String name;
        BufferView source;
        DXGI_FORMAT format;
        math::uint2 size;
        uint mips;
    };

    struct Texture {
        IndexOf<Image> image;

        // TODO: sampler
    };

    struct Material {
        static constexpr IndexType kType = eMaterial;
        static constexpr sm::StringView kName = "Material";

        sm::String name;

        float3 albedo;
        Texture albedo_texture;
    };

    struct Object {
        uint32 vtx_count;
        uint32 idx_count;

        DXGI_FORMAT index_format;

        BufferView vertices;
        BufferView indices;
    };

    struct PointLight {
        float3 colour;
        float intensity;
    };

    struct SpotLight {
        radf3 direction;
        float3 colour;
        float intensity;
    };

    struct DirectionalLight {
        radf3 direction;
        float3 colour;
        float intensity;
    };

    struct Light {
        static constexpr IndexType kType = eLight;
        static constexpr sm::StringView kName = "Light";

        sm::String name;
        sm::Variant<PointLight, SpotLight, DirectionalLight> light;
    };

    // struct PointLight2 {
    //     static constexpr IndexType kType = eLight;
    //     static constexpr sm::StringView kName = "PointLight";

    //     float3 position;
    //     float3 colour;
    //     float intensity;
    // };

    struct Model {
        static constexpr IndexType kType = eModel;
        static constexpr sm::StringView kName = "Model";

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
        static constexpr IndexType kType = eNode;
        static constexpr sm::StringView kName = "Node";

        IndexOf<Node> parent;

        sm::String name;

        Transform transform;

        sm::Vector<IndexOf<Node>> children;
        sm::Vector<IndexOf<Model>> models;
        sm::Vector<IndexOf<Light>> lights;
    };

    struct Camera {
        static constexpr IndexType kType = eCamera;
        static constexpr sm::StringView kName = "Camera";

        sm::String name;

        float3 position;
        float3 direction;

        radf fov;
    };

    struct Scene {
        static constexpr IndexType kType = eScene;
        static constexpr sm::StringView kName = "Scene";

        sm::String name;

        IndexOf<Node> root;
        IndexOf<Camera> camera;
    };

    using AnyIndex = ChoiceOf<Scene, Node, Camera, Model, File, Light, Buffer, Material, Image>;
    using OptionIndex = OptionOf<Scene, Node, Camera, Model, File, Light, Buffer, Material, Image>;

    struct World {
        sm::String name;
        IndexOf<Scene> default_scene;
        IndexOf<Material> default_material;

        sm::Vector<Scene> scenes;
        sm::Vector<Node> nodes;
        sm::Vector<Light> lights;
        sm::Vector<Camera> cameras;
        sm::Vector<Model> models;
        sm::Vector<File> files;
        sm::Vector<Buffer> buffers;
        sm::Vector<Material> materials;
        sm::Vector<Image> images;
        sm::Vector<Texture> textures;

        void move_node(IndexOf<Node> node, IndexOf<Node> parent);
        void clone_node(IndexOf<Node> node, IndexOf<Node> parent);

        template<typename T>
        IndexOf<T> clone(IndexOf<T> index) {
            T clone = get<T>(index);
            return add(std::move(clone));
        }

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

        template<typename T>
        const sm::Vector<T>& all() const { return get_vector<T>(); }

        auto visit(AnyIndex index, auto&& fn) {
            return std::visit([&](auto index) { return fn(get(index)); }, index);
        }

        template<typename T>
        sm::Vector<T>& get_vector();

        template<> sm::Vector<Node>& get_vector() { return nodes; }
        template<> sm::Vector<Camera>& get_vector() { return cameras; }
        template<> sm::Vector<Scene>& get_vector() { return scenes; }
        template<> sm::Vector<Light>& get_vector() { return lights; }
        template<> sm::Vector<Model>& get_vector() { return models; }
        template<> sm::Vector<File>& get_vector() { return files; }
        template<> sm::Vector<Buffer>& get_vector() { return buffers; }
        template<> sm::Vector<Material>& get_vector() { return materials; }
        template<> sm::Vector<Image>& get_vector() { return images; }
        template<> sm::Vector<Texture>& get_vector() { return textures; }
    };

    World default_world(sm::String name);

    bool load_world(World& info, Archive& archive);
    void save_world(Archive& archive, const World& world);
}

template<typename T>
struct std::hash<sm::world::IndexOf<T>> { // NOLINT
    size_t operator()(sm::world::IndexOf<T> index) const {
        return std::hash<sm::uint16>{}(index.get());
    }
};
