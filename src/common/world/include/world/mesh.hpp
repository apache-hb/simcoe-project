#pragma once

#include "core/vector.hpp"
#include "math/math.hpp"

#include "world.reflect.h"

namespace sm::world {
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
        float radius; // radius of the capsule part
        float height; // height of the cylinder part
        int slices; // number of vertical slices
        int rings; // number of rings to make the top and bottom of the capsule
    };

    struct Diamond {
        float width;
        float height;
        float depth;
    };

    struct GeoSphere {
        float radius;
        int subdivisions;
    };

    struct MeshInfo {
        ObjectType type;
        union {
            Cube cube;
            Sphere sphere;
            Cylinder cylinder;
            Plane plane;
            Wedge wedge;
            Capsule capsule;
            Diamond diamond;
            GeoSphere geosphere;
        };
    };

    struct BoxBounds {
        float3 min;
        float3 max;
    };

    struct Transform {
        float3 position;
        radf3 rotation; // TODO: use quaternion
        float3 scale;

        float4x4 matrix() const;
    };

    Transform default_transform();

    struct Mesh {
        BoxBounds bounds;
        sm::Vector<Vertex> vertices;
        sm::Vector<uint16> indices;
    };

    Mesh primitive(const MeshInfo& info);

    Mesh primitive(const Cube& cube);
    Mesh primitive(const Sphere& sphere);
    Mesh primitive(const Cylinder& cylinder);
    Mesh primitive(const Plane& plane);
    Mesh primitive(const Wedge& wedge);
    Mesh primitive(const Capsule& capsule);
    Mesh primitive(const Diamond& diamond);
    Mesh primitive(const GeoSphere& geosphere);
}
