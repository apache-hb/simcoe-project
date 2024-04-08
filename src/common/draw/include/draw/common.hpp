#pragma once

#ifndef __HLSL_VERSION
#   include "math/math.hpp"
namespace sm::draw {
using namespace sm::math;
#endif

struct ObjectData {
    float4x4 worldViewProjection;
    float4x4 worldView;
    float4x4 world;
};

struct ViewportData {
    float4x4 projection;
    float4x4 invProjection;
    float3 cameraPosition;
    float alphaTest;
    uint2 window;
    uint2 depthBufferSize;
    uint depthBufferSampleCount;
    uint pointLightCount;
    uint spotLightCount;

    uint window_width() { return window.x; }
    uint window_height() { return window.y; }

    uint depth_width() { return depthBufferSize.x; }
    uint depth_height() { return depthBufferSize.y; }
};

struct TileLightData {
    uint pointLightCount;
    uint spotLightCount;
};

struct LightVolumeData {
    float3 position;
    float radius;
};

struct PointLightData {
    float3 colour;
};

struct SpotLightData {
    float3 direction;
    float3 colour;
    float angle;
};

struct MaterialData {
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

#ifndef __HLSL_VERSION
} // namespace sm::draw
#endif
