#pragma once

#include "math/math.hpp"

#include "world/mesh.hpp"

#include <variant>

#include <flecs.h>

namespace sm::world::ecs {
    /// lights
    struct Light {};
    struct PointLight {};
    struct SpotLight {};

    struct Colour { math::float3 colour; };
    struct Direction { math::float3 direction; };
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

    struct World {};
    struct Local {};

    struct Position { math::float3 position; };
    struct Rotation { math::quatf rotation; };
    struct Scale { math::float3 scale; };

    struct AABB { math::float3 min; math::float3 max; };

    /// materials
    struct Material {
        math::float3 albedo;
    };

    /// cameras
    struct Camera {
        DXGI_FORMAT colour;
        DXGI_FORMAT depth;
        math::uint2 window;
        math::radf fov;

        float getAspectRatio() const { return float(window.x) / float(window.y); }
        math::float4x4 getProjectionMatrix() const;
    };

    void initSystems(flecs::world& world);

    math::float4x4 getViewMatrix(Position position, Direction direction);

    AABB bounds(world::Cube cube);
    AABB bounds(world::Sphere sphere);
    AABB bounds(world::Cylinder cylinder);
    AABB bounds(world::Plane plane);
    AABB bounds(world::Wedge wedge);
    AABB bounds(world::Capsule capsule);
}
