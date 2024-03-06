#pragma once

#include "core/vector.hpp"
#include "core/core.hpp"

#include "math/math.hpp"

#include "draw.reflect.h"

namespace sm::draw {
    using namespace math;

    constexpr float3 kVectorForward = {1.f, 0.f, 0.f};
    constexpr float3 kVectorRight = {0.f, 1.f, 0.f};
    constexpr float3 kVectorUp = {0.f, 0.f, 1.f};

    struct Vertex {
        float3 position;
        uint32_t colour;
    };

    struct Cube {
        float width;
        float height;
        float depth;
    };

    struct Sphere {
        float radius;
        int slices;
        int stacks;
    };

    struct Cylinder {
        float radius;
        float height;
        int slices;
    };

    struct Plane {
        float width;
        float depth;
    };

    struct Wedge {
        float width;
        float height;
        float depth;
    };

    struct Capsule {
        float radius;
        float height;
    };

    struct GeoSphere {
        float radius;
        int subdivisions;
    };

    struct MeshInfo {
        MeshType type;
        union {
            Cube cube;
            Sphere sphere;
            Cylinder cylinder;
            Plane plane;
            Wedge wedge;
            Capsule capsule;
            GeoSphere geosphere;
        };
    };

    struct BoxBounds {
        float3 min;
        float3 max;
    };

    struct Transform {
        float3 position;
        quatf rotation;
        float3 scale = 1.f;

        float4x4 matrix() const;
    };

    struct Mesh {
        BoxBounds bounds;
        sm::Vector<Vertex> vertices;
        sm::Vector<uint16> indices;
    };

    Mesh primitive(const MeshInfo& info);
}
