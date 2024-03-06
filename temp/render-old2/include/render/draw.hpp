#pragma once

#include "render/mesh.hpp"

namespace sm::draw {
    using namespace sm::math;

    struct Transform {
        float3 position;
        float3 rotation;
        float3 scale;

        float4x4 matrix() const;
    };

    struct ArrayModifier {
        uint32_t repeat;
        Transform transform;
    };

    struct Modifier {
        ModifierType type;
        union {
            ArrayModifier array;
        };
    };

    struct RenderNode {
        Transform transform;

        sm::SmallVector<uint16_t, 4> children;
        sm::SmallVector<uint16_t, 4> modifiers;
        sm::SmallVector<uint16_t, 4> meshes;
    };

    struct Camera {
        float3 mPosition;
        float3 forward;
        float3 up;

        float near_plane = 0.1f;
        float far_plane = 100.f;
        float fov = 90.f;

        float4x4 model() const;
        float4x4 view() const;
        float4x4 perspective(float width, float height) const;

        void move(float3 delta);
        void look(float3 delta);
    };

    struct Scene {
        uint16_t root;
        sm::Vector<RenderNode> nodes;
        sm::Vector<Modifier> modifiers;
        sm::Vector<MeshInfo> meshes;
    };
}
