#include "common.hlsli"

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
RWBuffer<uint> gLightIndexBuffer : register(u0);
RWStructuredBuffer<TileLightData> gTileLightBuffer : register(u1);

// shared memory for light culling

#if DEPTH_BOUNDS_MODE != DEPTH_BOUNDS_DISABLED
groupshared uint gZMax;
groupshared uint gZMin;
#endif

groupshared uint gLightIndexCount;
groupshared uint gLightIndex[MAX_LIGHTS_PER_TILE];

// constants

#define CS_THREADS_X TILE_SIZE
#define CS_THREADS_Y TILE_SIZE
#define THREADS_PER_TILE (CS_THREADS_X * CS_THREADS_Y)

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

struct FrustumData {
    float3 plane0;
    float3 plane1;
    float3 plane2;
    float3 plane3;

    bool test_frustum_sides(float3 position, float radius) {
        bool inside0 = signed_distance_from_plane(position, plane0) < radius;
        bool inside1 = signed_distance_from_plane(position, plane1) < radius;
        bool inside2 = signed_distance_from_plane(position, plane2) < radius;
        bool inside3 = signed_distance_from_plane(position, plane3) < radius;

        return all(bool4(inside0, inside1, inside2, inside3));
    }

    bool depth_test(float dist, float radius) {
#if DEPTH_BOUNDS_MODE != DEPTH_BOUNDS_DISABLED
        // TODO: implement
        return false;
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
    }
};

// light culling and binning compute shader

[numthreads(CS_THREADS_X, CS_THREADS_Y, 1)]
void csCullLights(
    uint3 globalId : SV_DispatchThreadID,
    uint3 localId : SV_GroupThreadID,
    uint3 groupId : SV_GroupID)
{
    uint localIndex = localId.x + localId.y * CS_THREADS_X;

    if (localIndex == 0)
        gLightIndexCount = 0;

    uint tile_count = get_tile_count();

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

    // TODO: support DEPTH_BOUNDS_MODE != DEPTH_BOUNDS_DISABLED

    // cull point lights for this tile
    frustum.cull_lights(localIndex, gCameraData.pointLightCount, gPointLightData);
    GroupMemoryBarrierWithGroupSync();
    uint pointLightsInTile = gLightIndexCount;

    // cull spot lights for this tile
    frustum.cull_lights(localIndex, gCameraData.spotLightCount, gSpotLightData);
    GroupMemoryBarrierWithGroupSync();
    uint spotLightsInTile = gLightIndexCount - pointLightsInTile;

    // write the light indices to the buffer
    uint globalTileIndex = groupId.x + groupId.y * tile_count.x;
    uint start = gCameraData.maxLightsPerTile * globalTileIndex;

    {
        for (uint i = globalTileIndex; i < pointLightsInTile; i += THREADS_PER_TILE) {
            gLightIndexBuffer[start + i] = gLightIndex[i];
        }
    }

    {
        for (uint j = globalTileIndex + pointLightsInTile; j < gLightIndexCount; j += THREADS_PER_TILE) {
            gLightIndexBuffer[start + j] = gLightIndex[j];
        }
    }

    // write out the light counts for this tile
    if (localIndex == 0) {
        TileLightData tileInfo = { pointLightsInTile, spotLightsInTile };
        gTileLightBuffer[globalTileIndex] = tileInfo;
    }
}
