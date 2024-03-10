#pragma once

#include "world/mesh.hpp"

#include "archive/archive.hpp"

namespace sm::world {
    using vtxindex = uint16; // NOLINT

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

    struct WorldInfo {
        sm::String name;
        uint16 root_node;
        uint16 active_camera;

        sm::Vector<NodeInfo> nodes;
        sm::Vector<CameraInfo> cameras;
        sm::Vector<ObjectInfo> objects;
        sm::Vector<MaterialInfo> materials;
        sm::Vector<ImageInfo> images;
        sm::Vector<BufferInfo> buffers;

        uint16 add_node(NodeInfo info);
        uint16 add_camera(CameraInfo info);

        void reparent_node(uint16 node, uint16 parent);
        void add_object(uint16 node, uint16 object);
    };

    WorldInfo empty_world(sm::StringView name);

    bool load_world(WorldInfo& info, Archive& archive);
    void save_world(Archive& archive, const WorldInfo& world);
}
