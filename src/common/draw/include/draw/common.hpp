#pragma once

#ifndef __HLSL_VERSION
#   include "math/math.hpp"
#   define row_major
#   define column_major
namespace sm::draw {
using namespace sm::math;
#endif

#ifdef __HLSL_VERSION
#   define CXX_CONSTEXPR
#   define CXX_CONST
#else
#   define CXX_CONSTEXPR constexpr
#   define CXX_CONST const
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

CXX_CONSTEXPR int cxxCeil(float x) {
#ifdef __cplusplus
    const int i = int(x);
    return (x > i) ? i + 1 : i;
#else
    return ceil(x);
#endif
}

CXX_CONSTEXPR int cxxFloor(float x) {
#ifdef __cplusplus
    const int i = int(x);
    return i - (i > x);
#else
    return floor(x);
#endif
}

CXX_CONSTEXPR uint2 computeGridSize(uint2 windowSize, uint tileSize) {
    uint x = (uint)((windowSize.x + tileSize - 1) / (float)(tileSize));
    uint y = (uint)((windowSize.y + tileSize - 1) / (float)(tileSize));

    return uint2(x, y);
}

// gets the size of the window rounded up to the next multiple of the tile size
CXX_CONSTEXPR uint2 computeWindowTiledSize(uint2 windowSize, uint tileSize) {
    uint2 gridSize = computeGridSize(windowSize, tileSize);
    return gridSize * tileSize;
}

CXX_CONSTEXPR uint computeTileCount(uint2 windowSize, uint tileSize) {
    uint2 grid = computeGridSize(windowSize, tileSize);
    return grid.x * grid.y;
}

CXX_CONSTEXPR uint computePixelTileIndex(uint2 windowSize, float2 position, uint tileSize) {
    uint2 grid = computeGridSize(windowSize, tileSize);

    uint row = cxxFloor(position.x / float(tileSize));
    uint column = cxxFloor(position.y / float(tileSize)) * grid.x;
    uint index = row + column;

    return index;
}

CXX_CONSTEXPR uint computeGroupTileIndex(uint3 groupId, uint2 windowSize, uint tileSize) {
    uint2 grid = computeGridSize(windowSize, tileSize);

    uint row = groupId.x;
    uint column = groupId.y * grid.x;
    uint groupIndex = row + column;

    return groupIndex;
}

typedef uint light_index_t;

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
    uint2 windowSize;
    uint2 depthBufferSize;
    uint depthBufferSampleCount;
    uint pointLightCount;
    uint spotLightCount;

    CXX_CONSTEXPR uint2 getWindowSize() CXX_CONST { return windowSize; }
    CXX_CONSTEXPR uint getWindowWidth() CXX_CONST { return windowSize.x; }
    CXX_CONSTEXPR uint getWindowHeight() CXX_CONST { return windowSize.y; }

    CXX_CONSTEXPR uint2 getDepthBufferSize() CXX_CONST { return depthBufferSize; }
    CXX_CONSTEXPR uint getDepthBufferWidth() CXX_CONST { return depthBufferSize.x; }
    CXX_CONSTEXPR uint getDepthBufferHeight() CXX_CONST { return depthBufferSize.y; }

    CXX_CONSTEXPR uint2 getGridSize(uint tileSize) CXX_CONST {
        return computeGridSize(getWindowSize(), tileSize);
    }

    CXX_CONSTEXPR uint getTileCount(uint tileSize) CXX_CONST {
        return computeTileCount(getWindowSize(), tileSize);
    }

    CXX_CONSTEXPR uint getPixelTileIndex(float2 position, uint tileSize) CXX_CONST {
        return computePixelTileIndex(getWindowSize(), position, tileSize);
    }

    CXX_CONSTEXPR uint getGroupTileIndex(uint3 groupThreadIndex, uint tileSize) CXX_CONST {
        return computeGroupTileIndex(groupThreadIndex, getWindowSize(), tileSize);
    }

    CXX_CONSTEXPR uint2 getWindowTiledSize(uint tileSize) CXX_CONST {
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

#ifndef __HLSL_VERSION
} // namespace sm::draw
#endif
