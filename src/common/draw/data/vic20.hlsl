#include "draw/common/vic20.h"

StructuredBuffer<Vic20Info> gDrawInfo : register(t0);

// framebuffer is a 1D array of colour palette indices of size `VIC20_FRAMEBUFFER_SIZE`
// each index is a 4 bit index into `kVic20Pallete`
StructuredBuffer<uint> gFrameBuffer : register(t1);

// a texture to write into with a resolution of `gTextureSize`
RWTexture2D<float4> gPresentTexture : register(u0);

// this is dispatched based on the size of `gTextureSize` rather than the framebuffer size
[numthreads(VIC20_THREADS_X, VIC20_THREADS_Y, 1)]
void csMain(uint3 dispatchId : SV_DispatchThreadID) {
    Vic20Info info = gDrawInfo[0];

    // index into the framebuffer array as if it was an array of uint8_t
    uint index = dispatchId.y * VIC20_SCREEN_WIDTH + dispatchId.x; // getFrameBufferIndex(dispatchId.xy, info.dispatchSize);

    uint paletteIndex = 0;

    if (index < VIC20_FRAMEBUFFER_SIZE) {
        paletteIndex = gFrameBuffer[index] & 0x0000000F;
    }

    float3 colour = kVic20Palette[paletteIndex];

    gPresentTexture[dispatchId.xy] = float4(colour, 1.f);
}
