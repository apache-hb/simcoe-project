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
