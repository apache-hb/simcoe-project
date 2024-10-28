#include "draw/common/vic20.h"

#define CS_THREADS_X 16
#define CS_THREADS_Y 16

StructuredBuffer<Vic20Info> gDrawInfo : register(t0);

// framebuffer is a 1D array of colour palette indices of size `VIC20_FRAMEBUFFER_SIZE`
// each index is a 4 bit index into `kVic20Pallete`
StructuredBuffer<uint> gFrameBuffer : register(t1);

// a texture to write into with a resolution of `gTextureSize`
RWTexture2D<float4> gPresentTexture : register(u0);

// this is dispatched based on the size of `gTextureSize` rather than the framebuffer size
[numthreads(CS_THREADS_X, CS_THREADS_Y, 1)]
void csMain(
    uint3 groupId : SV_GroupID,
    uint3 threadId : SV_GroupThreadID
) {
    // calculate the texture coordinates this thread will write to
    uint x = groupId.x * CS_THREADS_X + threadId.x;
    uint y = groupId.y * CS_THREADS_Y + threadId.y;

    // TODO: the rest of the function
    gPresentTexture[uint2(x, y)] = float4(kVic20Pallete[gFrameBuffer[0]], 1.f);
}
