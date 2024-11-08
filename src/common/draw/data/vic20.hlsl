#include "draw/common/vic20.h"

StructuredBuffer<Vic20Info> gDrawInfo : register(t0);

StructuredBuffer<Vic20CharacterMap> gCharacterMap : register(t1);

StructuredBuffer<Vic20Screen> gScreenState : register(t2);

// a texture to write into with a resolution of `gTextureSize`
RWTexture2D<float4> gPresentTexture : register(u0);

#define X_PIXELS_PER_WORD (8)

uint getCharacterIndex(Vic20Screen screen, uint index) {
    uint elementIndex = index / sizeof(uint);
    uint byteIndex = index % sizeof(uint);
    if (elementIndex >= VIC20_SCREEN_CHARBUFFER_SIZE)
        return VIC20_CHARMAP_SIZE;

    return screen.screen[elementIndex] >> (byteIndex * 8) & 0xFF;
}

uint getCharacterColour(Vic20Screen screen, uint index) {
    uint elementIndex = index / sizeof(uint);
    uint byteIndex = index % sizeof(uint);
    if (elementIndex >= VIC20_SCREEN_CHARBUFFER_SIZE)
        return 0xFF;

    return screen.colour[elementIndex] >> (byteIndex * 8) & 0xFF;
}

uint foregroundColour(uint colour) {
    return (colour >> 4) & 0x0F;
}

uint backgroundColour(uint colour) {
    return colour & 0x0F;
}

float4 getPaletteColourFromIndex(uint index) {
    if (index >= VIC20_PALETTE_SIZE)
        return float4(1, 0, 1, 1);

    return float4(kVic20Palette[index], 1.f);
}

uint getScreenIndex(uint2 dispatchId, float2 textureSize) {
    float uvX = float(dispatchId.x) / textureSize.x;
    float uvY = float(dispatchId.y) / textureSize.y;

    uint fbX = floor(uvX * VIC20_SCREEN_CHARS_WIDTH);
    uint fbY = floor(uvY * VIC20_SCREEN_CHARS_HEIGHT);

    return fbY * VIC20_SCREEN_CHARS_WIDTH + fbX;
}

// this is dispatched based on the size of `gTextureSize` rather than the framebuffer size
[numthreads(VIC20_THREADS_X / X_PIXELS_PER_WORD, VIC20_THREADS_Y, 1)]
void csMain(
    uint3 groupId : SV_GroupID,
    uint3 groupThreadId : SV_GroupThreadID,
    uint3 dispatchId : SV_DispatchThreadID
) {
    Vic20Info info = gDrawInfo[0];
    Vic20CharacterMap characters = gCharacterMap[0];
    Vic20Screen screen = gScreenState[0];

    uint2 threadId = groupId.xy * uint2(VIC20_THREADS_X, VIC20_THREADS_Y) + groupThreadId.xy * uint2(X_PIXELS_PER_WORD, 1);
    uint index = getScreenIndex(threadId, info.textureSize) / X_PIXELS_PER_WORD;
    uint characterIndex = getCharacterIndex(screen, index);
    uint colour = getCharacterColour(screen, index);
    uint fg = foregroundColour(colour);
    uint bg = backgroundColour(colour);

    gPresentTexture[threadId + uint2(0, 0)] = getPaletteColourFromIndex(fg);
    gPresentTexture[threadId + uint2(1, 0)] = getPaletteColourFromIndex(fg);
    gPresentTexture[threadId + uint2(2, 0)] = getPaletteColourFromIndex(fg);
    gPresentTexture[threadId + uint2(3, 0)] = getPaletteColourFromIndex(fg);
    gPresentTexture[threadId + uint2(4, 0)] = getPaletteColourFromIndex(bg);
    gPresentTexture[threadId + uint2(5, 0)] = getPaletteColourFromIndex(bg);
    gPresentTexture[threadId + uint2(6, 0)] = getPaletteColourFromIndex(bg);
    gPresentTexture[threadId + uint2(7, 0)] = getPaletteColourFromIndex(bg);
}
