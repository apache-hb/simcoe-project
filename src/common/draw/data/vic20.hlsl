#include "draw/common/vic20.h"

StructuredBuffer<Vic20Info> gDrawInfo : register(t0);

// framebuffer is a 1D array of colour palette indices of size `VIC20_SCREEN_SIZE`
// each index is a 4 bit index into `kVic20Pallete`
StructuredBuffer<uint> gFrameBuffer : register(t1);

// a texture to write into with a resolution of `gTextureSize`
RWTexture2D<float4> gPresentTexture : register(u0);

uint getPaletteIndexValue(uint index) {
    if (index >= VIC20_SCREEN_SIZE)
        return VIC20_PALETTE_SIZE;

    return gFrameBuffer[index];
}

float4 getPaletteColourFromIndex(uint index) {
    if (index >= VIC20_PALETTE_SIZE)
        return float4(1, 0, 1, 1);

    return float4(kVic20Palette[index], 1.f);
}

float4 getPaletteColour(uint index) {
    if (index >= VIC20_SCREEN_SIZE)
        return float4(1, 0, 1, 1);

    uint paletteIndex = gFrameBuffer[index];
    if (paletteIndex >= VIC20_PALETTE_SIZE)
        return float4(1, 1, 0, 1);

    return float4(kVic20Palette[paletteIndex], 1.f);
}

uint getIndexForDispatch(uint2 dispatchId, float2 textureSize) {
    float shaderWidth = textureSize.x;
    float shaderHeight = textureSize.y;

    float uvX = float(dispatchId.x) / shaderWidth;
    float uvY = float(dispatchId.y) / shaderHeight;

    uint fbX = uvX * VIC20_SCREEN_WIDTH;
    uint fbY = uvY * VIC20_SCREEN_HEIGHT;

    return fbY * VIC20_SCREEN_WIDTH + fbX;
}

// this is dispatched based on the size of `gTextureSize` rather than the framebuffer size
[numthreads(VIC20_THREADS_X / 2, VIC20_THREADS_Y, 1)]
void csMain(
    uint3 groupId : SV_GroupID,
    uint3 groupThreadId : SV_GroupThreadID
) {
    Vic20Info info = gDrawInfo[0];

    for (uint i = 0; i <= 2; i++) {
        uint2 threadId = groupId.xy * uint2(VIC20_THREADS_X, VIC20_THREADS_Y) + groupThreadId.xy + uint2(i * 2, 0);
        uint index = getIndexForDispatch(threadId, info.textureSize) / 2;
        uint colourIndexPair = getPaletteIndexValue(index);
        uint low = colourIndexPair & 0xFFFF;
        uint high = colourIndexPair >> 16;

        gPresentTexture[threadId + uint2(0, 0)] = getPaletteColourFromIndex(low);
        gPresentTexture[threadId + uint2(1, 0)] = getPaletteColourFromIndex(high);
    }
}
