#pragma once

#include "core/vector.hpp"
#include "core/core.hpp"

#include "math/math.hpp"

#include "world/world.hpp"

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

    struct BoxBounds {
        float3 min;
        float3 max;
    };

    struct Mesh {
        BoxBounds bounds;
        sm::Vector<Vertex> vertices;
        sm::Vector<uint16> indices;
    };

    Mesh primitive(const world::MeshInfo& info);
}
