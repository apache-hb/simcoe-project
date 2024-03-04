#pragma once

#include "world/mesh.hpp"

namespace sm::world {
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
    };

    WorldInfo empty_world(sm::StringView name);

    WorldInfo load_world(Archive& archive);
    void save_world(Archive& archive, const WorldInfo& world);

    WorldInfo load_gltf(const fs::Path& path);
}
