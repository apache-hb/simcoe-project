#pragma once

#include "core/variant.hpp"

#include "world/mesh.hpp"

#include "archive/archive.hpp"

namespace sm::world {
    using vtxindex = uint16; // NOLINT

    template<typename T>
    class IndexOf {
        uint16 mIndex;
    };

    struct NodeInfo {
        sm::String name;
        sm::Vector<uint16> children;
        sm::Vector<uint16> objects;

        Transform transform;

        // transient
        uint16 parent;
    };

    struct CameraInfo {
        sm::String name;

        float3 position;
        float3 direction;
    };

    struct ObjectInfo {
        sm::String name;
        MeshInfo info;
    };

    struct MaterialInfo {
        sm::String name;
    };

    struct ImageInfo {
        sm::String name;
    };

    struct BufferInfo {
        sm::String name;
        sm::Vector<byte> data;
    };

    struct File {
        sm::String path;
    };

    struct Buffer {
        sm::String name;
        sm::Vector<uint8> data;
    };

    struct BufferView {
        sm::Variant<File, Buffer> source;

        uint64 offset;
        uint32 file_size;
        uint32 buffer_size;
    };

    struct MeshObject {
        uint32 vtx_count;
        uint32 idx_count;
        BufferView vertices;
        BufferView indices;
    };

    struct MeshData {
        sm::Variant<
            MeshObject,
            Cube,
            Sphere,
            Cylinder,
            Plane,
            Wedge,
            Capsule,
            Diamond,
            GeoSphere
        > data;
    };

    struct Model {
        sm::String name;
        MeshData mesh;
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

        sm::Vector<IndexOf<Camera>> cameras;
    };

    struct WorldInfo {
        sm::String name;
        uint16 root_node;
        uint16 active_camera;
        uint16 default_material;

        sm::Vector<NodeInfo> nodes;
        sm::Vector<CameraInfo> cameras;
        sm::Vector<ObjectInfo> objects;
        sm::Vector<MaterialInfo> materials;
        sm::Vector<ImageInfo> images;
        sm::Vector<BufferInfo> buffers;

        uint16 add_node(const NodeInfo& info);
        uint16 add_camera(const CameraInfo& info);
        uint16 add_object(const ObjectInfo& info);
        uint16 add_material(const MaterialInfo& info);

        void reparent_node(uint16 node, uint16 parent);
        void delete_node(uint16 index);
        bool is_root_node(uint16 node) const;
        void add_node_object(uint16 node, uint16 object);
    };

    WorldInfo empty_world(sm::StringView name);

    bool load_world(WorldInfo& info, Archive& archive);
    void save_world(Archive& archive, const WorldInfo& world);
}
