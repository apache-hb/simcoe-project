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

    uint word = colourValueData >> (dispatchId.x % 2 == 0 ? 0 : 16);

    for (uint i = 0; i < X_PIXELS_PER_WORD; i += 2) {
        uint value = (word >> (i * 2)) & 0xF;
        float4 colour = getPaletteColourFromIndex(value);
        gPresentTexture[threadId + uint2(i, 0)] = colour;
        gPresentTexture[threadId + uint2(i + 1, 0)] = colour;
    }
}
