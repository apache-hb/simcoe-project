#pragma once

#include "math/math.hpp"

#include "world/mesh.hpp"

#include <variant>

namespace sm::world::ecs {
    /// lights
    struct Light {};
    struct PointLight {};
    struct SpotLight {};

    struct Colour { math::float3 colour; };
    struct Direction { math::quatf direction; };
    struct Intensity { float intensity; };

    /// objects
    struct Object {};
    struct Shape {
        std::variant<
            world::Cube,
            world::Sphere,
            world::Cylinder,
            world::Plane,
            world::Wedge,
            world::Capsule
        > info;
    };

    struct Position { math::float3 position; };
    struct Rotation { math::quatf rotation; };
    struct Scale { math::float3 scale; };

    struct AABB { math::float3 min; math::float3 max; };

    /// materials
    struct Material {
        math::float3 albedo;
    };

    /// cameras
    struct Camera {};
    struct Perspective { math::radf fov; };
    struct Orthographic { float width; float height; };

    AABB bounds(world::Cube cube);
    AABB bounds(world::Sphere sphere);
    AABB bounds(world::Cylinder cylinder);
    AABB bounds(world::Plane plane);
    AABB bounds(world::Wedge wedge);
    AABB bounds(world::Capsule capsule);
}
