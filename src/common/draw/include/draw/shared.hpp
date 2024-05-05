#pragma once

#include "math/math.hpp"

#include "draw/common.hpp"

namespace sm::draw {
    ///
    /// everything else
    ///

    struct Material {
        math::float3 albedo;
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
