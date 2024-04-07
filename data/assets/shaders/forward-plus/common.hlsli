#include "../types.hlsli"

// configuration

#define DEPTH_BOUNDS_DISABLED 0
#define DEPTH_BOUNDS_ENABLED 1
#define DEPTH_BOUNDS_MSAA 2

#ifndef DEPTH_BOUNDS_MODE
#   define DEPTH_BOUNDS_MODE DEPTH_BOUNDS_DISABLED
#endif

#define LIGHT_CULLING_DISABLED 0
#define LIGHT_CULLING_ENABLED 1

#ifndef LIGHT_CULLING_MODE
#   define LIGHT_CULLING_MODE LIGHT_CULLING_DISABLED
#endif

#define ALPHA_TEST_DISABLED 0
#define ALPHA_TEST_ENABLED 1

#ifndef ALPHA_TEST_MODE
#   define ALPHA_TEST_MODE ALPHA_TEST_DISABLED
#endif

#ifndef MAX_LIGHTS_PER_TILE
#   define MAX_LIGHTS_PER_TILE 256
#endif

#ifndef TILE_SIZE
#   define TILE_SIZE 16
#endif

// TODO: get this working
// * use uint16_t for light indices, counts, window size, depth texture size, etc
// * test everything

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
    uint2 depth;
    uint pointLightCount;
    uint spotLightCount;
    uint maxLightsPerTile;

    uint window_width() { return window.x; }
    uint window_height() { return window.y; }

    uint depth_width() { return depth.x; }
    uint depth_height() { return depth.y; }
};

struct TileLightData {
    uint pointLightCount;
    uint spotLightCount;
};

cbuffer ObjectBuffer : register(b0) {
    ObjectData gObjectData;
};

cbuffer FrameBuffer : register(b1) {
    ViewportData gCameraData;
};

uint get_tile_index(float2 screen) {
    float res = float(TILE_SIZE);
    uint cells_x = (gCameraData.window_width() + TILE_SIZE - 1) / TILE_SIZE;
    uint tile_idx = floor(screen.x / res) + floor(screen.y / res) * cells_x;

    return tile_idx;
}
