#pragma once

#include "flecs.h" // IWYU pragma: export
#include "math/math.hpp"

namespace sm::game {
    struct Position { math::float3 position; };
    struct Scale { math::float3 scale; };
    struct Rotation { math::quatf rotation; };

    struct BoxBounds {
        math::float3 min;
        math::float3 max;
    };

    struct MeshBuffer { };

    struct Cube {
        float width;
        float height;
        float depth;
    };

    struct Cylinder {
        float radius;
        float height;
        int slices;
    };

    struct Wedge {
        float width;
        float height;
        float depth;
    };

    struct Capsule {
        float radius;
        float height;
        int slices;
        int rings;
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

    struct Mesh { };

    struct Material {

    };

    struct Image {
        math::float4 colour;
    };

    void init(flecs::world& ecs);
}
