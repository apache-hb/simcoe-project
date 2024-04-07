#pragma once

#include "math/math.hpp"

namespace sm::render {
    using namespace sm::math;

    // TODO: merge these headers
    // keep in sync with shaders/common.hlsli

    struct alignas(256) ObjectData {
        float4x4 world_view_projection;
        float4x4 world_view;
        float4x4 world;
    };

    struct alignas(256) ViewportData {
        float4x4 projection;
        float4x4 inv_projection;
        float3 camera_position;
        float alpha_test;
        uint width;
        uint height;
        uint point_light_count;
        uint spot_light_count;
    };

    struct TileLightData {
        uint point_light_count;
        uint spot_light_count;
    };

    struct LightVolumeData {
        float3 position;
        float radius;
    };

    struct PointLight {
        float3 colour;
    };

    struct SpotLight {
        float3 direction;
        float3 colour;
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

    constexpr uint get_tile_count(uint2 size, uint tile) {
        return ((size.x + tile - 1) / tile) * ((size.y + tile - 1) / tile);
    }

    constexpr uint get_max_lights(uint tiles, uint lights) {
        return tiles * lights;
    }
}
