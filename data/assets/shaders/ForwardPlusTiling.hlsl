#define MAX_LIGHTS 512
#define TILE_SIZE 16

#define THREADS_X TILE_SIZE
#define THREADS_Y TILE_SIZE

struct Light {
    float3 position;
    float radius;
};

struct FrameData {
    float4x4 projection;
    float4x4 iprojection;
    uint2 resolution;
    uint lights;
};

cbuffer FrameDataBuffer : register(b0) {
    FrameData gFrameData;
};

StructuredBuffer<Light> gLights : register(t0);

groupshared uint gLightCount = 0;
groupshared uint gLightIndices[MAX_LIGHTS];

uint tile_count_x()
{
    return ((uint)(gFrameData.resolution.x + TILE_SIZE - 1) / TILE_SIZE);
}

uint tile_count_y()
{
    return ((uint)(gFrameData.resolution.y + TILE_SIZE - 1) / TILE_SIZE);
}

[numthreads(THREADS_X, THREADS_Y, 1)]
void cull_lights(uint3 global_id : SV_DispatchThreadID,
            uint3 local_id : SV_GroupThreadID,
            uint3 group_id : SV_GroupID)
{
    float3 frustum_eqn0, frustum_eqn1, frustum_eqn2, frustum_eqn3;
    {
        uint pxm = TILE_SIZE * group_id.x;
        uint pym = TILE_SIZE * group_id.y;
        uint pxp = TILE_SIZE * (group_id.x + 1);
        uint pyp = TILE_SIZE * (group_id.y + 1);

        uint width = TILE_SIZE * tile_count_x();
        uint height = TILE_SIZE * tile_count_y();
    }
}
