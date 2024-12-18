#pragma once

#include "hlsl-prelude.hpp"

INTEROP_BEGIN(sm::draw)

// render configuration
#define MAX_OBJECTS 2048
#define MAX_VIEWPORTS 16
#define MAX_LIGHTS 0x1000

#define MAX_POINT_LIGHTS (MAX_LIGHTS / 2)
#define MAX_SPOT_LIGHTS  (MAX_LIGHTS / 2)

#define POINT_LIGHT_FIRST_INDEX (0)
#define SPOT_LIGHT_FIRST_INDEX  (MAX_POINT_LIGHTS)

// forward+ rendering configuration
#define TILE_SIZE 16
#define MAX_LIGHTS_PER_TILE 512

#define MAX_POINT_LIGHTS_PER_TILE (MAX_LIGHTS_PER_TILE / 2)
#define MAX_SPOT_LIGHTS_PER_TILE  (MAX_LIGHTS_PER_TILE / 2)

#define DEPTH_BOUNDS_DISABLED 0
#define DEPTH_BOUNDS_ENABLED 1
#define DEPTH_BOUNDS_MSAA 2

#define ALPHA_TEST_DISABLED 0
#define ALPHA_TEST_ENABLED 1

typedef uint light_index_t;
typedef uint texture_index_t;

struct LightIndexData {
    light_index_t pointLightCount;
    light_index_t spotLightCount;
    light_index_t pointLightIndices[MAX_POINT_LIGHTS_PER_TILE];
    light_index_t spotLightIndices[MAX_SPOT_LIGHTS_PER_TILE];
};

#define LIGHT_INDEX_BUFFER_HEADER (1 + 1)
#define LIGHT_INDEX_BUFFER_STRIDE (LIGHT_INDEX_BUFFER_HEADER + MAX_POINT_LIGHTS_PER_TILE + MAX_SPOT_LIGHTS_PER_TILE)

CXX(static_assert(sizeof(LightIndexData) == LIGHT_INDEX_BUFFER_STRIDE * sizeof(light_index_t)));

constexpr int cxxCeil(float x) noexcept {
#ifdef __cplusplus
    const int i = static_cast<int>(x);
    return (x > i) ? i + 1 : i;
#else
    return ceil(x);
#endif
}

constexpr int cxxFloor(float x) noexcept {
#ifdef __cplusplus
    const int i = static_cast<int>(x);
    return (x < i) ? i - 1 : i;
#else
    return floor(x);
#endif
}

constexpr uint2 computeGridSize(uint2 windowSize, uint tileSize) noexcept {
    CXX(CTASSERTF(sm::isPowerOf2(tileSize), "tile size must be a power of 2"));

    uint x = (uint)((windowSize.x + tileSize - 1) / (float)(tileSize));
    uint y = (uint)((windowSize.y + tileSize - 1) / (float)(tileSize));

    return uint2(x, y);
}

// gets the size of the window rounded up to the next multiple of the tile size
constexpr uint2 computeWindowTiledSize(uint2 windowSize, uint tileSize) noexcept {
    uint2 gridSize = computeGridSize(windowSize, tileSize);
    return gridSize * tileSize;
}

constexpr uint computeTileCount(uint2 windowSize, uint tileSize) noexcept {
    uint2 grid = computeGridSize(windowSize, tileSize);
    return grid.x * grid.y;
}

constexpr uint computePixelTileIndex(uint2 windowSize, float2 position, uint tileSize) noexcept {
    uint2 grid = computeGridSize(windowSize, tileSize);

    uint row = cxxFloor(position.x / float(tileSize));
    uint column = cxxFloor(position.y / float(tileSize)) * grid.x;
    uint index = row + column;

    return index;
}

constexpr uint computeGroupTileIndex(uint3 groupId, uint2 windowSize, uint tileSize) noexcept {
    uint2 grid = computeGridSize(windowSize, tileSize);

    uint row = groupId.x;
    uint column = groupId.y * grid.x;
    uint groupIndex = row + column;

    return groupIndex;
}

struct ObjectData {
    float4x4 model;
};

struct CXX(alignas(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)) ViewportData {
    float4x4 viewProjection;
    float4x4 worldView;

    float4x4 projection;
    float4x4 invProjection;
    float3 cameraPosition;
    float alphaTest;
    uint2 windowSize;
    uint2 depthBufferSize;
    uint depthBufferSampleCount;
    uint pointLightCount;
    uint spotLightCount;

    constexpr uint2 getWindowSize() CXX(const) noexcept { return windowSize; }
    constexpr uint getWindowWidth() CXX(const) noexcept { return windowSize.x; }
    constexpr uint getWindowHeight() CXX(const) noexcept { return windowSize.y; }

    constexpr uint2 getDepthBufferSize() CXX(const) noexcept { return depthBufferSize; }
    constexpr uint getDepthBufferWidth() CXX(const) noexcept { return depthBufferSize.x; }
    constexpr uint getDepthBufferHeight() CXX(const) noexcept { return depthBufferSize.y; }

    constexpr uint2 getGridSize(uint tileSize) CXX(const) noexcept {
        return computeGridSize(getWindowSize(), tileSize);
    }

    constexpr uint getTileCount(uint tileSize) CXX(const) noexcept {
        return computeTileCount(getWindowSize(), tileSize);
    }

    constexpr uint getPixelTileIndex(float2 position, uint tileSize) CXX(const) noexcept {
        return computePixelTileIndex(getWindowSize(), position, tileSize);
    }

    constexpr uint getGroupTileIndex(uint3 groupThreadIndex, uint tileSize) CXX(const) noexcept {
        return computeGroupTileIndex(groupThreadIndex, getWindowSize(), tileSize);
    }

    constexpr uint2 getWindowTiledSize(uint tileSize) CXX(const) noexcept {
        return computeWindowTiledSize(getWindowSize(), tileSize);
    }
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

    float metallic;
    float specular;
    float roughness;

    texture_index_t albedTextureIndex;
    texture_index_t metallicTextureIndex;
    texture_index_t specularTextureIndex;
    texture_index_t roughnessTextureIndex;
    texture_index_t normalTextureIndex;
};

constexpr float signedDistanceFromPlane(float3 plane, float3 eqn) {
    return dot(plane, eqn);
}

constexpr float3 createPlaneEquation(float3 b, float3 c) {
    return normalize(cross(b, c));
}

constexpr float4 createProjection(uint2 windowSize, uint px, uint py) {
    float x = px / float(windowSize.x) * 2 - 1;
    float y = (windowSize.y - py) / float(windowSize.y) * 2 - 1;
    return float4(x, y, 1, 1);
}

constexpr uint getLocalIndex(uint3 groupThreadId, uint threadCountX) {
    return groupThreadId.x + groupThreadId.y * threadCountX;
}

constexpr uint getTileIndexCount(uint2 windowSize, uint tileSize) {
    uint tileCount = computeTileCount(windowSize, tileSize);
    return (tileCount * LIGHT_INDEX_BUFFER_STRIDE) + 1;
}

INTEROP_END(sm::draw)

#include "hlsl-epilog.hpp" // IWYU pragma: keep
