#include "common.hlsli"

// constants

#define CS_THREADS_X TILE_SIZE
#define CS_THREADS_Y TILE_SIZE
#define THREADS_PER_TILE (CS_THREADS_X * CS_THREADS_Y)

// all light data for binning/culling

StructuredBuffer<LightVolumeData> gPointLightData : register(t0);
StructuredBuffer<LightVolumeData> gSpotLightData : register(t1);

// texture for depth bounds testing
#if DEPTH_BOUNDS_MODE == DEPTH_BOUNDS_MSAA
Texture2DMS<float> gDepthTexture : register(t2);
#elif DEPTH_BOUNDS_MODE == DEPTH_BOUNDS_ENABLED
Texture2D<float> gDepthTexture : register(t2);
#endif

// TileLightData[tileGridSize.x * tileGridSize.y]
// output buffer for light indices
// struct PointLightData {
//   uint pointLightCount;
//   uint spotLightCount;
//   uint pointLightIndices[MAX_POINT_LIGHTS_PER_TILE];
//   uint spotLightIndices[MAX_SPOT_LIGHTS_PER_TILE];
// };
RWBuffer<light_index_t> gLightIndexBuffer : register(u0);

// shared memory for light culling

#if DEPTH_BOUNDS_MODE != DEPTH_BOUNDS_DISABLED
groupshared uint ldsZMax;
groupshared uint ldsZMin;
#endif

groupshared light_index_t gLightIndexCount;
groupshared light_index_t gLightIndex[MAX_LIGHTS_PER_TILE];

// helper functions

uint2 getWindowGridSize() {
    return gCameraData.getGridSize(TILE_SIZE);
}

float3 convertProjectionToView(float4 p, float4x4 invProjection) {
    float4 v = mul(p, invProjection);
    v /= v.w;
    return v.xyz;
}

float convertProjectionDepthToView(float depth, float4x4 invProjection) {
    return 1.f / (depth * invProjection._34 + invProjection._44);
}

#if DEPTH_BOUNDS_MODE == DEPTH_BOUNDS_ENABLED

void computeDepthBounds(uint3 globalId) {
    float depth = gDepthTexture.Load(uint3(globalId.xy, 0)).x;
    float viewPositionZ = convertProjectionDepthToView(depth, gCameraData.invProjection);
    uint z = asuint(viewPositionZ);
    if (depth != 0.f) {
        InterlockedMax(ldsZMax, z);
        InterlockedMin(ldsZMin, z);
    }
}

#elif DEPTH_BOUNDS_MODE == DEPTH_BOUNDS_MSAA

void computeDepthBoundsMSAA(uint3 globalId, uint depthSampleCount) {
    float pixelMinZ = FLOAT_MAX;
    float pixelMaxZ = 0.f;

    float depth = gDepthTexture.Load(globalId.xy, 0).x;
    float viewPositionZ0 = convertProjectionDepthToView(depth, gCameraData.invProjection);
    if (depth != 0.f) {
        pixelMaxZ = max(pixelMaxZ, viewPositionZ0);
        pixelMinZ = min(pixelMinZ, viewPositionZ0);
    }

    for (uint i = 1; i < depthSampleCount; i++) {
        depth = gDepthTexture.Load(globalId.xy, i).x;
        float viewPositionZ = convertProjectionDepthToView(depth, gCameraData.invProjection);
        if (depth != 0.f) {
            pixelMaxZ = max(pixelMaxZ, viewPositionZ);
            pixelMinZ = min(pixelMinZ, viewPositionZ);
        }
    }

    uint zMin = asuint(pixelMinZ);
    uint zMax = asuint(pixelMaxZ);

    InterlockedMax(ldsZMax, zMax);
    InterlockedMin(ldsZMin, zMin);
}

#endif

struct FrustumData {
    float3 plane0;
    float3 plane1;
    float3 plane2;
    float3 plane3;

#if DEPTH_BOUNDS_MODE != DEPTH_BOUNDS_DISABLED
    float minZ;
    float maxZ;

    void updateFromLDS() {
        GroupMemoryBarrierWithGroupSync();
        minZ = asfloat(ldsZMin);
        maxZ = asfloat(ldsZMax);
    }
#endif

    bool testFrustrumSides(float3 position, float radius) {
        bool inside0 = signedDistanceFromPlane(position, plane0) < radius;
        bool inside1 = signedDistanceFromPlane(position, plane1) < radius;
        bool inside2 = signedDistanceFromPlane(position, plane2) < radius;
        bool inside3 = signedDistanceFromPlane(position, plane3) < radius;

        return all(bool4(inside0, inside1, inside2, inside3));
    }

    bool depthTest(float dist, float radius) {
#if DEPTH_BOUNDS_MODE != DEPTH_BOUNDS_DISABLED
        return -dist + minZ < radius && dist - maxZ < radius;
#else
        return dist >= radius;
#endif
    }

    void cullLights(uint localIndex, uint count, StructuredBuffer<LightVolumeData> data) {
        for (uint i = localIndex; i < count; i += THREADS_PER_TILE) {
            LightVolumeData light = data[i];

            float3 center = mul(float4(light.position, 1), gCameraData.worldView).xyz;

            if (testFrustrumSides(center, light.radius)) {
                if (depthTest(center.z, light.radius)) {
                    uint index = 0;
                    InterlockedAdd(gLightIndexCount, 1, index);
                    gLightIndex[index] = i;
                }
            }
        }

        GroupMemoryBarrierWithGroupSync();
    }
};

FrustumData createFrustumData(uint2 groupId) {
    uint pxm = TILE_SIZE * groupId.x;
    uint pym = TILE_SIZE * groupId.y;
    uint pxp = TILE_SIZE * (groupId.x + 1);
    uint pyp = TILE_SIZE * (groupId.y + 1);

    uint2 windowSize = gCameraData.getWindowTiledSize(TILE_SIZE);

    float3 frustum0 = convertProjectionToView(createProjection(windowSize, pxm, pym), gCameraData.invProjection);
    float3 frustum1 = convertProjectionToView(createProjection(windowSize, pxp, pym), gCameraData.invProjection);
    float3 frustum2 = convertProjectionToView(createProjection(windowSize, pxp, pyp), gCameraData.invProjection);
    float3 frustum3 = convertProjectionToView(createProjection(windowSize, pxm, pyp), gCameraData.invProjection);

    FrustumData frustum;
    frustum.plane0 = createPlaneEquation(frustum0, frustum1);
    frustum.plane1 = createPlaneEquation(frustum1, frustum2);
    frustum.plane2 = createPlaneEquation(frustum2, frustum3);
    frustum.plane3 = createPlaneEquation(frustum3, frustum0);
    return frustum;
}

void doLightCulling(uint3 globalId, uint3 localId, uint3 groupId) {
    uint localIndex = localId.x + localId.y * CS_THREADS_X;

    if (localIndex == 0) {
#if DEPTH_BOUNDS_MODE != DEPTH_BOUNDS_DISABLED
        ldsZMax = 0;
        ldsZMin = 0x7f7fffff;
#endif
        gLightIndexCount = 0;
    }

    FrustumData frustum = createFrustumData(groupId.xy);

    GroupMemoryBarrierWithGroupSync();

#if DEPTH_BOUNDS_MODE == DEPTH_BOUNDS_ENABLED
    computeDepthBounds(globalId);

    frustum.updateFromLDS();
#elif DEPTH_BOUNDS_MODE == DEPTH_BOUNDS_MSAA
    computeDepthBoundsMSAA(globalId, gCameraData.depthBufferSampleCount);

    frustum.updateFromLDS();
#endif

    // cull point lights for this tile
    frustum.cullLights(localIndex, gCameraData.pointLightCount, gPointLightData);
    light_index_t pointLightsInTile = gLightIndexCount;

    // cull spot lights for this tile
    frustum.cullLights(localIndex, gCameraData.spotLightCount, gSpotLightData);
    light_index_t spotLightsInTile = gLightIndexCount - pointLightsInTile;

    GroupMemoryBarrierWithGroupSync();

    // write the light indices to the buffer
    uint groupIdIndex = gCameraData.getGroupTileIndex(groupId, TILE_SIZE);
    uint startOffset = groupIdIndex * LIGHT_INDEX_BUFFER_STRIDE;

#if 1

    if (startOffset >= 2621400) {
        const int scale = 32;
        uint2 gridSize = getWindowGridSize();
        gLightIndexBuffer[(groupIdIndex * scale) + 0] = 0xFFFFFFFF;

        gLightIndexBuffer[(groupIdIndex * scale) + 1] = groupId.x;
        gLightIndexBuffer[(groupIdIndex * scale) + 2] = groupId.y;
        gLightIndexBuffer[(groupIdIndex * scale) + 3] = groupId.z;

        gLightIndexBuffer[(groupIdIndex * scale) + 4] = localId.x;
        gLightIndexBuffer[(groupIdIndex * scale) + 5] = localId.y;
        gLightIndexBuffer[(groupIdIndex * scale) + 6] = localId.z;

        gLightIndexBuffer[(groupIdIndex * scale) + 7] = globalId.x;
        gLightIndexBuffer[(groupIdIndex * scale) + 8] = globalId.y;
        gLightIndexBuffer[(groupIdIndex * scale) + 9] = globalId.z;

        gLightIndexBuffer[(groupIdIndex * scale) + 10] = gCameraData.windowSize.x;
        gLightIndexBuffer[(groupIdIndex * scale) + 11] = gCameraData.windowSize.y;
        gLightIndexBuffer[(groupIdIndex * scale) + 12] = gridSize.x;
        gLightIndexBuffer[(groupIdIndex * scale) + 13] = gridSize.y;

        gLightIndexBuffer[(groupIdIndex * scale) + 14] = TILE_SIZE;
        gLightIndexBuffer[(groupIdIndex * scale) + 15] = THREADS_PER_TILE;
        gLightIndexBuffer[(groupIdIndex * scale) + 16] = LIGHT_INDEX_BUFFER_STRIDE;

        gLightIndexBuffer[(groupIdIndex * scale) + scale - 1] = 0xFFFFFFFF;
        return;
    }
#endif

    // copy the light indices to the buffer
    {
        for (uint i = 0; i < pointLightsInTile; i += THREADS_PER_TILE) {
            gLightIndexBuffer[startOffset + LIGHT_INDEX_BUFFER_HEADER + i] = gLightIndex[i];
        }
    }

    {
        for (uint j = 0; j < gLightIndexCount; j += THREADS_PER_TILE) {
            gLightIndexBuffer[startOffset + LIGHT_INDEX_BUFFER_HEADER + MAX_POINT_LIGHTS_PER_TILE + j] = gLightIndex[j + pointLightsInTile];
        }
    }


    // save the light counts for this tile
    if (localIndex == 0) {
        gLightIndexBuffer[startOffset + 0] = pointLightsInTile;
        gLightIndexBuffer[startOffset + 1] = spotLightsInTile;
    }
}

// light culling and binning compute shader

[numthreads(CS_THREADS_X, CS_THREADS_Y, 1)]
void csCullLights(
    uint3 globalId : SV_DispatchThreadID,
    uint3 localId : SV_GroupThreadID,
    uint3 groupId : SV_GroupID
)
{
    doLightCulling(globalId, localId, groupId);
}
