#pragma once

#include "core/vector.hpp"
#include "core/core.hpp"

#include "math/math.hpp"

#include "draw.reflect.h"

namespace sm::draw {
    using namespace sm::math;

    struct Cube {
        float width;
        float height;
        float depth;
    };

    struct Sphere {
        float radius;
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
        float depth;
        float height;
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

    struct Vertex {
        float3 position;
        float2 uv;
    };

    struct BoxBounds {
        float3 min;
        float3 max;
    };

    struct Mesh {
        BoxBounds bounds;
        sm::Vector<Vertex> vertices;
        sm::Vector<uint16> indices;
    };

    Mesh primitive(const MeshInfo& info);
}
