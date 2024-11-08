#include "draw/common/vic20.h"

StructuredBuffer<Vic20Info> gDrawInfo : register(t0);

StructuredBuffer<Vic20CharacterMap> gCharacterMap : register(t1);

StructuredBuffer<Vic20Screen> gScreenState : register(t2);

// a texture to write into with a resolution of `gTextureSize`
RWTexture2D<float4> gPresentTexture : register(u0);

#define X_PIXELS_PER_THREAD (8)
#define Y_PIXELS_PER_THREAD (8)

uint getCharacterIndex(Vic20Screen screen, uint index) {
    if (index >= VIC20_SCREEN_CHARBUFFER_SIZE)
        return VIC20_CHARMAP_SIZE;

    uint elementIndex = index / sizeof(ScreenElement);
    uint byteIndex = index % sizeof(ScreenElement);

    return screen.screen[elementIndex] >> (byteIndex * 8) & 0xFF;
}

Vic20Character getCharacter(Vic20CharacterMap characters, uint index) {
    if (index >= VIC20_CHARMAP_SIZE) {
        return Vic20Character(0b0101010101010101010101010101010101010101010101010101010101010101);
    }

    return characters.characters[index];
}

uint getCharacterColour(Vic20Screen screen, uint index) {
    if (index >= VIC20_SCREEN_CHARBUFFER_SIZE)
        return VIC20_CHAR_COLOUR(VIC20_COLOUR_PINK, VIC20_COLOUR_BLACK);

    uint elementIndex = index / sizeof(ScreenElement);
    uint byteIndex = index % sizeof(ScreenElement);

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

uint getScreenIndex(uint2 dispatchId) {
    return dispatchId.y * VIC20_SCREEN_CHARS_WIDTH + dispatchId.x;
}

uint2 getTextureIndex(uint2 dispatchId, float2 textureSize) {
    float uvX = float(dispatchId.x) / VIC20_SCREEN_CHARS_WIDTH;
    float uvY = float(dispatchId.y) / VIC20_SCREEN_CHARS_HEIGHT;

    uint fbX = floor(uvX * textureSize.x);
    uint fbY = floor(uvY * textureSize.y);

    return uint2(fbX, fbY);
}

// each thread is responsible for a single pixel in a character
[numthreads(VIC20_THREADS_X, VIC20_THREADS_Y, 1)]
void csMain(
    uint3 groupId : SV_GroupID,
    uint3 groupThreadId : SV_GroupThreadID,
    uint3 dispatchId : SV_DispatchThreadID
) {
    Vic20Info info = gDrawInfo[0];
    Vic20CharacterMap characters = gCharacterMap[0];
    Vic20Screen screen = gScreenState[0];

    uint groupIndex = groupThreadId.y * VIC20_THREADS_X + groupThreadId.x;

    uint index = getScreenIndex(groupId.xy);
    uint characterIndex = getCharacterIndex(screen, index);
    Vic20Character character = getCharacter(characters, characterIndex);
    bool pickForeground = (character.data & (1ULL << groupIndex)) != 0;
    uint colour = getCharacterColour(screen, index);

    uint finalColour = pickForeground
        ? foregroundColour(colour)
        : backgroundColour(colour);

    uint2 pixel = groupId.xy * uint2(VIC20_THREADS_X, VIC20_THREADS_Y) + groupThreadId.xy;
    gPresentTexture[pixel] = getPaletteColourFromIndex(finalColour);

    // float ic = float(index) / VIC20_SCREEN_CHARBUFFER_SIZE;
    // float u = float(threadId.x) / info.textureSize.x;
    // float v = float(threadId.y) / info.textureSize.y;

    // gPresentTexture[threadId + uint2(1, 0)] = getPaletteColourFromIndex(fg);
    // gPresentTexture[threadId + uint2(2, 0)] = getPaletteColourFromIndex(fg);
    // gPresentTexture[threadId + uint2(3, 0)] = getPaletteColourFromIndex(fg);
    // gPresentTexture[threadId + uint2(4, 0)] = getPaletteColourFromIndex(bg);
    // gPresentTexture[threadId + uint2(5, 0)] = getPaletteColourFromIndex(bg);
    // gPresentTexture[threadId + uint2(6, 0)] = getPaletteColourFromIndex(bg);
    // gPresentTexture[threadId + uint2(7, 0)] = getPaletteColourFromIndex(bg);
}
