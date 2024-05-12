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

// struct PointLightData {
//   uint pointLightCount;
//   uint spotLightCount;
//   uint pointLightIndices[MAX_POINT_LIGHTS_PER_TILE];
//   uint spotLightIndices[MAX_SPOT_LIGHTS_PER_TILE];
// };
#define LIGHT_INDEX_BUFFER_HEADER (1 + 1)
#define LIGHT_INDEX_BUFFER_STRIDE (LIGHT_INDEX_BUFFER_HEADER + MAX_LIGHTS_PER_TILE)

typedef uint light_index_t;

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
    uint albedo_texture;

    float metallic;
    uint metallic_texture;

    float specular;
    uint specular_texture;

    float roughness;
    uint roughness_texture;

    uint normal_texture;
};

#ifdef __cplusplus
// simple case:
// - window size: (1920, 1080)
// - tile size: 16
struct TestCase0 {
    static constexpr uint2 kWindowSize = uint2(1920, 1080);
    static constexpr uint kTileSize = 16;

    static constexpr uint2 kExpectedGridSize = uint2(120, 68);
    static constexpr uint kExpectedTileCount = 8160;

    static_assert(computeGridSize(kWindowSize, kTileSize) == uint2(120, 68));
    static_assert(computeTileCount(kWindowSize, kTileSize) == kExpectedTileCount);

    static_assert(computeWindowTiledSize(kWindowSize, kTileSize) == kExpectedGridSize * kTileSize);

    static_assert(computePixelTileIndex(kWindowSize, float2(0, 0), kTileSize) == 0);
    static_assert(computePixelTileIndex(kWindowSize, float2(kWindowSize), kTileSize) == kExpectedTileCount);

    static_assert(computeGroupTileIndex(uint3(0, 0, 0), kWindowSize, kTileSize) == 0);
    static_assert(computeGroupTileIndex(uint3(1, 0, 0), kWindowSize, kTileSize) == 1);
    static_assert(computeGroupTileIndex(uint3(0, 1, 0), kWindowSize, kTileSize) == kExpectedGridSize.x);
    static_assert(computeGroupTileIndex(uint3(16, 16, 0), kWindowSize, kTileSize) == 1936);

    // TODO: the <= comparison is technically wrong, it means we may skip the bottom right tile
    static_assert(computeGroupTileIndex(uint3(kExpectedGridSize - 1, 0), kWindowSize, kTileSize) <= kExpectedTileCount);
};

// case 1:
// - window size: (2119, 860)
// - tile size: 16
struct TestCase1 {
    static constexpr uint2 kWindowSize = uint2(2119, 860);
    static constexpr uint kTileSize = 16;

    static constexpr uint2 kExpectedGridSize = uint2(133, 54);
    static constexpr uint kExpectedTileCount = 7182;

    static_assert(computeGridSize(kWindowSize, kTileSize) == kExpectedGridSize);
    static_assert(computeTileCount(kWindowSize, kTileSize) == kExpectedTileCount);

    static_assert(computeWindowTiledSize(kWindowSize, kTileSize) == kExpectedGridSize * kTileSize);

    static_assert(computePixelTileIndex(kWindowSize, float2(0, 0), kTileSize) == 0);
    static_assert(computePixelTileIndex(kWindowSize, float2(kWindowSize), kTileSize) <= kExpectedTileCount);

    static_assert(computeGroupTileIndex(uint3(0, 0, 0), kWindowSize, kTileSize) == 0);
    static_assert(computeGroupTileIndex(uint3(1, 0, 0), kWindowSize, kTileSize) == 1);
    static_assert(computeGroupTileIndex(uint3(0, 1, 0), kWindowSize, kTileSize) == kExpectedGridSize.x);
    static_assert(computeGroupTileIndex(uint3(kExpectedGridSize - 1, 0), kWindowSize, kTileSize) <= kExpectedTileCount);
};

// case 2:
// - window size: (2119, 902)
// - tile size: 16
struct TestCase2 {
    static constexpr uint2 kWindowSize = uint2(2119, 902);
    static constexpr uint kTileSize = 16;

    static constexpr uint2 kExpectedGridSize = uint2(133, 57);
    static constexpr uint kExpectedTileCount = 7581;

    static_assert(computeGridSize(kWindowSize, kTileSize) == kExpectedGridSize);
    static_assert(computeTileCount(kWindowSize, kTileSize) == kExpectedTileCount);

    static_assert(computeWindowTiledSize(kWindowSize, kTileSize) == kExpectedGridSize * kTileSize);

    static_assert(computePixelTileIndex(kWindowSize, float2(0, 0), kTileSize) == 0);
    static_assert(computePixelTileIndex(kWindowSize, float2(kWindowSize), kTileSize) <= kExpectedTileCount);

    static_assert(computeGroupTileIndex(uint3(0, 0, 0), kWindowSize, kTileSize) == 0);
    static_assert(computeGroupTileIndex(uint3(1, 0, 0), kWindowSize, kTileSize) == 1);
    static_assert(computeGroupTileIndex(uint3(0, 1, 0), kWindowSize, kTileSize) == kExpectedGridSize.x);
    static_assert(computeGroupTileIndex(uint3(kExpectedGridSize - 1, 0), kWindowSize, kTileSize) <= kExpectedTileCount);

    static_assert(computeTileCount(kWindowSize, kTileSize) * LIGHT_INDEX_BUFFER_STRIDE == 3896634);
};
#endif

INTEROP_END(sm::draw)

#include "hlsl-epilog.hpp"
