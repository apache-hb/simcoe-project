#pragma once

#ifndef __HLSL_VERSION
#   include "math/math.hpp"
#   define row_major
#   define column_major
namespace sm::draw {
using namespace sm::math;
#endif

#define TILE_SIZE 16

#define MAX_LIGHTS 0x1000

#define MAX_POINT_LIGHTS (MAX_LIGHTS / 2)
#define MAX_SPOT_LIGHTS  (MAX_LIGHTS / 2)

#ifndef MAX_LIGHTS_PER_TILE
#   define MAX_LIGHTS_PER_TILE 512
#endif

#define MAX_POINT_LIGHTS_PER_TILE (MAX_LIGHTS_PER_TILE / 2)
#define MAX_SPOT_LIGHTS_PER_TILE  (MAX_LIGHTS_PER_TILE / 2)

// struct PointLightData {
//   uint pointLightCount;
//   uint spotLightCount;
//   uint pointLightIndices[MAX_POINT_LIGHTS_PER_TILE];
//   uint spotLightIndices[MAX_SPOT_LIGHTS_PER_TILE];
// };
#define LIGHT_INDEX_BUFFER_HEADER (1 + 1)
#define LIGHT_INDEX_BUFFER_STRIDE (LIGHT_INDEX_BUFFER_HEADER + MAX_POINT_LIGHTS_PER_TILE + MAX_SPOT_LIGHTS_PER_TILE)

struct ObjectData {
    float4x4 model;
};

struct ViewportData {
    float4x4 viewProjection;
    float4x4 worldView;

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
