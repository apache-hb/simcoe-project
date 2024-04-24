#pragma once

#include "math/math.hpp"

#include "draw/common.hpp"

namespace sm::draw {
    /// @brief get the number of tiles required to cover the screen
    constexpr uint get_tile_count(uint2 size, uint tile) {
        return ((size.x + tile - 1) / tile) * ((size.y + tile - 1) / tile);
    }

    constexpr uint get_max_lights(uint tiles, uint lights) {
        return tiles * lights;
    }

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
