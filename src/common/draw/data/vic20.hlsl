#include "draw/common/vic20.h"

StructuredBuffer<Vic20Info> gDrawInfo : register(t0);

// framebuffer is a 1D array of colour palette indices of size `VIC20_SCREEN_SIZE`
// each index is a 4 bit index into `kVic20Pallete`
StructuredBuffer<uint> gFrameBuffer : register(t1);

// a texture to write into with a resolution of `gTextureSize`
RWTexture2D<float4> gPresentTexture : register(u0);

#define X_PIXELS_PER_WORD 8

uint getPaletteIndexValue(uint index) {
    if (index >= VIC20_FRAMEBUFFER_SIZE)
        return VIC20_PALETTE_SIZE;

    return gFrameBuffer[index];
}

float4 getPaletteColourFromIndex(uint index) {
    if (index >= VIC20_PALETTE_SIZE)
        return float4(1, 0, 1, 1);

    return float4(kVic20Palette[index], 1.f);
}

uint getIndexForDispatch(uint2 dispatchId, float2 textureSize) {
    float uvX = float(dispatchId.x) / textureSize.x;
    float uvY = float(dispatchId.y) / textureSize.y;

    uint fbX = uvX * VIC20_SCREEN_WIDTH;
    uint fbY = uvY * VIC20_SCREEN_HEIGHT;

    return fbY * VIC20_SCREEN_WIDTH + fbX;
}

// this is dispatched based on the size of `gTextureSize` rather than the framebuffer size
[numthreads(VIC20_THREADS_X / X_PIXELS_PER_WORD, VIC20_THREADS_Y, 1)]
void csMain(
    uint3 groupId : SV_GroupID,
    uint3 groupThreadId : SV_GroupThreadID
) {
    Vic20Info info = gDrawInfo[0];

    uint2 threadId = groupId.xy * uint2(VIC20_THREADS_X, VIC20_THREADS_Y) + groupThreadId.xy * uint2(8, 1);
    uint index = getIndexForDispatch(threadId, info.textureSize) / 8;
    uint colourValueData = getPaletteIndexValue(index);

    uint byte0 = (colourValueData >> 0)  & 0xF;
    uint byte1 = (colourValueData >> 4)  & 0xF;
    uint byte2 = (colourValueData >> 8)  & 0xF;
    uint byte3 = (colourValueData >> 12) & 0xF;
    uint byte4 = (colourValueData >> 16) & 0xF;
    uint byte5 = (colourValueData >> 20) & 0xF;
    uint byte6 = (colourValueData >> 24) & 0xF;
    uint byte7 = (colourValueData >> 28) & 0xF;

    gPresentTexture[threadId + uint2(0, 0)] = getPaletteColourFromIndex(byte0);
    gPresentTexture[threadId + uint2(1, 0)] = getPaletteColourFromIndex(byte1);
    gPresentTexture[threadId + uint2(2, 0)] = getPaletteColourFromIndex(byte2);
    gPresentTexture[threadId + uint2(3, 0)] = getPaletteColourFromIndex(byte3);
    gPresentTexture[threadId + uint2(4, 0)] = getPaletteColourFromIndex(byte4);
    gPresentTexture[threadId + uint2(5, 0)] = getPaletteColourFromIndex(byte5);
    gPresentTexture[threadId + uint2(6, 0)] = getPaletteColourFromIndex(byte6);
    gPresentTexture[threadId + uint2(7, 0)] = getPaletteColourFromIndex(byte7);
}
