#include "draw/common/vic20.h"

StructuredBuffer<Vic20Info> gDrawInfo : register(t0);

// framebuffer is a 1D array of colour palette indices of size `VIC20_SCREEN_SIZE`
// each index is a 4 bit index into `kVic20Pallete`
StructuredBuffer<uint> gFrameBuffer : register(t1);

// a texture to write into with a resolution of `gTextureSize`
RWTexture2D<float4> gPresentTexture : register(u0);

float4 getPaletteColour(uint index) {
    if (index >= VIC20_SCREEN_SIZE)
        return float4(1, 0, 1, 1);

    uint paletteIndex = gFrameBuffer[index] & VIC20_PALETTE_MASK;
    return float4(kVic20Palette[paletteIndex], 1.f);
}

// this is dispatched based on the size of `gTextureSize` rather than the framebuffer size
[numthreads(VIC20_THREADS_X, VIC20_THREADS_Y, 1)]
void csMain(
    uint3 groupId : SV_GroupID,
    uint3 groupThreadId : SV_GroupThreadID,
    uint3 dispatchId : SV_DispatchThreadID
) {
    Vic20Info info = gDrawInfo[0];

    float shaderWidth = info.dispatchSize.x * VIC20_THREADS_X;
    float shaderHeight = info.dispatchSize.y * VIC20_THREADS_Y;
    float fbX = (dispatchId.x / shaderWidth) * VIC20_SCREEN_WIDTH;
    float fbY = (dispatchId.y / shaderHeight) * VIC20_SCREEN_HEIGHT;

    uint index = fbY * VIC20_SCREEN_WIDTH + fbX;

    gPresentTexture[dispatchId.xy] = getPaletteColour(index);
}
