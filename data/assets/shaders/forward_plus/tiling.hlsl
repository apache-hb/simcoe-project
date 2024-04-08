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

// output buffer for light indices

// uint[MAX_LIGHTS_PER_TILE * tile_count.x * tile_count.y]
RWBuffer<uint> gLightIndexBuffer : register(u0);

// TileLightData[MAX_LIGHTS_PER_TILE * tile_count.x * tile_count.y]
RWStructuredBuffer<TileLightData> gTileLightBuffer : register(u1);

// shared memory for light culling

#if DEPTH_BOUNDS_MODE != DEPTH_BOUNDS_DISABLED
groupshared uint ldsZMax;
groupshared uint ldsZMin;
#endif

groupshared uint gLightIndexCount;
groupshared uint gLightIndex[MAX_LIGHTS_PER_TILE];

// helper functions

uint2 get_tile_count() {
    uint x = uint((gCameraData.window_width() + TILE_SIZE - 1) / float(TILE_SIZE));
    uint y = uint((gCameraData.window_height() + TILE_SIZE - 1) / float(TILE_SIZE));

    return uint2(x, y);
}

float3 convert_projection_to_view(float4 p) {
    float4 v = mul(p, gCameraData.invProjection);
    v /= v.w;
    return v.xyz;
}

float convert_projection_depth_to_view(float depth) {
    return 1.f / (depth * gCameraData.invProjection._34 + gCameraData.invProjection._44);
}

float3 create_plane_equation(float3 b, float3 c) {
    return normalize(cross(b, c));
}

float4 create_projection(uint2 window_size, uint px, uint py) {
    float x = px / float(window_size.x) * 2 - 1;
    float y = (window_size.y - py) / float(window_size.y) * 2 - 1;
    return float4(x, y, 1, 1);
}

float signed_distance_from_plane(float3 plane, float3 eqn) {
    return dot(plane, eqn);
}

#if DEPTH_BOUNDS_MODE == DEPTH_BOUNDS_ENABLED

void compute_depth_bounds(uint3 globalId) {
    float depth = gDepthTexture.LightVolumeData(uint3(globalId.xy, 0)).x;
    float viewPositionZ = convert_projection_depth_to_view(depth);
    uint z = asuint(viewPositionZ);
    if (depth != 0.f) {
        InterlockedMax(ldsZMax, z);
        InterlockedMin(ldsZMin, z);
    }
}

#elif DEPTH_BOUNDS_MODE == DEPTH_BOUNDS_MSAA

void compute_depth_bounds_msaa(uint3 globalId, uint depthSampleCount) {
    float pixelMinZ = FLOAT_MAX;
    float pixelMaxZ = 0.f;

    float depth = gDepthTexture.Load(globalId.xy, 0).x;
    float viewPositionZ0 = convert_projection_depth_to_view(depth);
    if (depth != 0.f) {
        pixelMaxZ = max(pixelMaxZ, viewPositionZ0);
        pixelMinZ = min(pixelMinZ, viewPositionZ0);
    }

    for (uint i = 1; i < depthSampleCount; i++) {
        depth = gDepthTexture.Load(globalId.xy, i).x;
        float viewPositionZ = convert_projection_depth_to_view(depth);
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

    void update_from_lds() {
        GroupMemoryBarrierWithGroupSync();
        minZ = asfloat(ldsZMin);
        maxZ = asfloat(ldsZMax);
    }
#endif

    bool test_frustum_sides(float3 position, float radius) {
        bool inside0 = signed_distance_from_plane(position, plane0) < radius;
        bool inside1 = signed_distance_from_plane(position, plane1) < radius;
        bool inside2 = signed_distance_from_plane(position, plane2) < radius;
        bool inside3 = signed_distance_from_plane(position, plane3) < radius;

        return all(bool4(inside0, inside1, inside2, inside3));
    }

    bool depth_test(float dist, float radius) {
#if DEPTH_BOUNDS_MODE != DEPTH_BOUNDS_DISABLED
        return -dist + minZ < radius && dist - maxZ < radius;
#else
        return dist >= radius;
#endif
    }

    void cull_lights(uint localIndex, uint count, StructuredBuffer<LightVolumeData> data) {
        for (uint i = localIndex; i < count; i += THREADS_PER_TILE) {
            LightVolumeData light = data[i];

            float3 center = mul(float4(light.position, 1), gObjectData.worldView).xyz;

            if (test_frustum_sides(center, light.radius)) {
                if (depth_test(center.z, light.radius)) {
                    uint index = 0;
                    InterlockedAdd(gLightIndexCount, 1, index);
                    gLightIndex[index] = i;
                }
            }
        }

        GroupMemoryBarrierWithGroupSync();
    }
};

// light culling and binning compute shader

[numthreads(CS_THREADS_X, CS_THREADS_Y, 1)]
void cs_cull_lights(
    uint3 globalId : SV_DispatchThreadID,
    uint3 localId : SV_GroupThreadID,
    uint3 groupId : SV_GroupID)
{
    uint localIndex = localId.x + localId.y * CS_THREADS_X;

    if (localIndex == 0) {
#if DEPTH_BOUNDS_MODE != DEPTH_BOUNDS_DISABLED
        ldsZMax = 0;
        ldsZMin = 0x7f7fffff;
#endif
        gLightIndexCount = 0;
    }

    uint2 tile_count = get_tile_count();

    FrustumData frustum;
    {
        uint pxm = TILE_SIZE * groupId.x;
        uint pym = TILE_SIZE * groupId.y;
        uint pxp = TILE_SIZE * (groupId.x + 1);
        uint pyp = TILE_SIZE * (groupId.y + 1);

        uint2 window_size = TILE_SIZE * tile_count;

        float3 frustum0 = convert_projection_to_view(create_projection(window_size, pxm, pym));
        float3 frustum1 = convert_projection_to_view(create_projection(window_size, pxp, pym));
        float3 frustum2 = convert_projection_to_view(create_projection(window_size, pxp, pyp));
        float3 frustum3 = convert_projection_to_view(create_projection(window_size, pxm, pyp));

        frustum.plane0 = create_plane_equation(frustum0, frustum1);
        frustum.plane1 = create_plane_equation(frustum1, frustum2);
        frustum.plane2 = create_plane_equation(frustum2, frustum3);
        frustum.plane3 = create_plane_equation(frustum3, frustum0);
    }

    GroupMemoryBarrierWithGroupSync();

#if DEPTH_BOUNDS_MODE == DEPTH_BOUNDS_ENABLED
    compute_depth_bounds(globalId);

    frustum.update_from_lds();
#elif DEPTH_BOUNDS_MODE == DEPTH_BOUNDS_MSAA
    compute_depth_bounds_msaa(globalId, gCameraData.depthBufferSampleCount);

    frustum.update_from_lds();
#endif

    // cull point lights for this tile
    frustum.cull_lights(localIndex, gCameraData.pointLightCount, gPointLightData);
    uint pointLightsInTile = gLightIndexCount;

    // cull spot lights for this tile
    frustum.cull_lights(localIndex, gCameraData.spotLightCount, gSpotLightData);
    uint spotLightsInTile = gLightIndexCount - pointLightsInTile;

    // write the light indices to the buffer
    uint globalTileIndex = groupId.x + groupId.y * tile_count.x;
    uint start = MAX_LIGHTS_PER_TILE * globalTileIndex;

    {
        for (uint i = globalTileIndex; i < pointLightsInTile; i += THREADS_PER_TILE) {
            gLightIndexBuffer[start + i] = gLightIndex[i];
        }
    }

    {
        for (uint j = globalTileIndex + MAX_POINT_LIGHTS_PER_TILE; j < gLightIndexCount; j += THREADS_PER_TILE) {
            gLightIndexBuffer[start + j] = gLightIndex[j];
        }
    }

    // write out the light counts for this tile
    if (localIndex == 0) {
        TileLightData tileInfo = { pointLightsInTile, spotLightsInTile };
        gTileLightBuffer[globalTileIndex] = tileInfo;
    }
}
