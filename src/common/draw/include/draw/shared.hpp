#pragma once

#include "math/math.hpp"

namespace sm::render {
    using namespace sm::math;

    struct PointLight {
        float3 position;
        float3 colour;
        float radius;
    };

    struct SpotLight {
        float3 position;
        float3 direction;
        float3 colour;
        float radius;
        float angle;
    };

    struct Material {
        float3 albedo;
        uint albedo_texture;

        float metallic;
        uint metallic_texture;

        float specular;
        uint specular_texture;

        float roughness;
        uint roughness_texture;

        uint normal_texture;
    };
}
