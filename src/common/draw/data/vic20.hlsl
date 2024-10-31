#include "draw/common/vic20.h"

StructuredBuffer<Vic20Info> gDrawInfo : register(t0);

typedef uint PixelData;

// framebuffer is a 1D array of colour palette indices of size `VIC20_SCREEN_SIZE`
// each index is a 4 bit index into `kVic20Pallete`
StructuredBuffer<PixelData> gFrameBuffer : register(t1);

// a texture to write into with a resolution of `gTextureSize`
RWTexture2D<float4> gPresentTexture : register(u0);

#define X_PIXELS_PER_WORD (sizeof(PixelData) * 2)

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

    uint fbX = floor(uvX * VIC20_SCREEN_WIDTH);
    uint fbY = floor(uvY * VIC20_SCREEN_HEIGHT);

    return fbY * VIC20_SCREEN_WIDTH + fbX;
}

// this is dispatched based on the size of `gTextureSize` rather than the framebuffer size
[numthreads(VIC20_THREADS_X / X_PIXELS_PER_WORD, VIC20_THREADS_Y, 1)]
void csMain(
    uint3 groupId : SV_GroupID,
    uint3 groupThreadId : SV_GroupThreadID,
    uint3 dispatchId : SV_DispatchThreadID
) {
    Vic20Info info = gDrawInfo[0];

    uint2 threadId = groupId.xy * uint2(VIC20_THREADS_X, VIC20_THREADS_Y) + groupThreadId.xy * uint2(X_PIXELS_PER_WORD, 1);
    uint index = getIndexForDispatch(threadId, info.textureSize) / X_PIXELS_PER_WORD;
    uint colourValueData = getPaletteIndexValue(index);

    uint word = colourValueData;

    uint nibble0 = (word >> 0) & 0xF;
    uint nibble1 = (word >> 4) & 0xF;
    uint nibble2 = (word >> 8) & 0xF;
    uint nibble3 = (word >> 12) & 0xF;
    uint nibble4 = (word >> 16) & 0xF;
    uint nibble5 = (word >> 20) & 0xF;
    uint nibble6 = (word >> 24) & 0xF;
    uint nibble7 = (word >> 28) & 0xF;

    gPresentTexture[threadId + uint2(0, 0)] = getPaletteColourFromIndex(nibble0);
    gPresentTexture[threadId + uint2(1, 0)] = getPaletteColourFromIndex(nibble1);
    gPresentTexture[threadId + uint2(2, 0)] = getPaletteColourFromIndex(nibble2);
    gPresentTexture[threadId + uint2(3, 0)] = getPaletteColourFromIndex(nibble3);
    gPresentTexture[threadId + uint2(4, 0)] = getPaletteColourFromIndex(nibble4);
    gPresentTexture[threadId + uint2(5, 0)] = getPaletteColourFromIndex(nibble5);
    gPresentTexture[threadId + uint2(6, 0)] = getPaletteColourFromIndex(nibble6);
    gPresentTexture[threadId + uint2(7, 0)] = getPaletteColourFromIndex(nibble7);
}
