#pragma once

#ifndef __HLSL_VERSION
#include "math/math.hpp"
namespace sm::draw::shared { using namespace sm::math;

#define IS_CONST_BUFFER alignas(256)
#define CXX_CONSTEXPR constexpr
#else
#define IS_CONST_BUFFER
#define CXX_CONSTEXPR
#endif

struct IS_CONST_BUFFER Vic20Info {
    uint2 textureSize;
    uint2 dispatchSize;
};

#define VIC20_THREADS_X 8
#define VIC20_THREADS_Y 8

#define VIC20_PALETTE_SIZE 16
#define VIC20_SCREEN_WIDTH 176
#define VIC20_SCREEN_HEIGHT 184
#define VIC20_BPP 4
#define VIC20_FRAMEBUFFER_SIZE ((VIC20_SCREEN_WIDTH * VIC20_SCREEN_HEIGHT * VIC20_BPP) / 8)

#define VIC20_COLOUR_BLACK 0
#define VIC20_COLOUR_WHITE 1
#define VIC20_COLOUR_DARK_BROWN 2
#define VIC20_COLOUR_LIGHT_BROWN 3

#define VIC20_COLOUR_MAROON 4
#define VIC20_COLOUR_LIGHT_RED 5
#define VIC20_COLOUR_BLUE 6
#define VIC20_COLOUR_LIGHT_BLUE 7

#define VIC20_COLOUR_PURPLE 8
#define VIC20_COLOUR_PINK 9
#define VIC20_COLOUR_GREEN 10
#define VIC20_COLOUR_LIGHT_GREEN 11

#define VIC20_COLOUR_DARK_PURPLE 12
#define VIC20_COLOUR_LIGHT_PURPLE 13
#define VIC20_COLOUR_YELLOW_GREEN 14
#define VIC20_COLOUR_LIGHT_YELLOW 15

#define VIC20_COLOUR(r, g, b) float3(r / 255.0f, g / 255.0f, b / 255.0f)

// https://lospec.com/palette-list/commodore-vic-20
static const float3 kVic20Palette[VIC20_PALETTE_SIZE] = {
    // #00'00'00
    VIC20_COLOUR(0, 0, 0),
    // #ff'ff'ff
    VIC20_COLOUR(0xff, 0xff, 0xff),
    // #a8'73'4a
    VIC20_COLOUR(0xa8, 0x73, 0x4a),
    // #e9'b2'87
    VIC20_COLOUR(0xe9, 0xb2, 0x87),
    // ##77'2d'26
    VIC20_COLOUR(0x77, 0x2d, 0x26),
    // #b6'68'62
    VIC20_COLOUR(0xb6, 0x68, 0x62),
    // #85'd4'dc
    VIC20_COLOUR(0x85, 0xd4, 0xdc),
    // #c5'ff'ff
    VIC20_COLOUR(0xc5, 0xff, 0xff),
    // #a8'5f'b4
    VIC20_COLOUR(0xa8, 0x5f, 0xb4),
    // #e9'9d'f5
    VIC20_COLOUR(0xe9, 0x9d, 0xf5),
    // #55'9e'4a
    VIC20_COLOUR(0x55, 0x9e, 0x4a),
    // #92'df'87
    VIC20_COLOUR(0x92, 0xdf, 0x87),
    // #42'34'8b
    VIC20_COLOUR(0x42, 0x34, 0x8b),
    // #7e'70'ca
    VIC20_COLOUR(0x7e, 0x70, 0xca),
    // #bd'cc'71
    VIC20_COLOUR(0xbd, 0xcc, 0x71),
    // #ff'ff'b0
    VIC20_COLOUR(0xff, 0xff, 0xb0),
};

CXX_CONSTEXPR uint2 getTextureCoords(uint2 groupId, uint2 threadId, uint2 numthreads) {
    return groupId * numthreads + threadId;
}

CXX_CONSTEXPR uint getFrameBufferX(uint dispatchIdX, uint dispatchSizeX) {
    return (dispatchIdX * VIC20_SCREEN_WIDTH) / (dispatchSizeX * VIC20_THREADS_X);
}

CXX_CONSTEXPR uint getFrameBufferY(uint dispatchIdY, uint dispatchSizeY) {
    return (dispatchIdY * VIC20_SCREEN_HEIGHT) / (dispatchSizeY * VIC20_THREADS_Y);
}

/// @brief get the byte index of the framebuffer for a given thread
/// @param groupId SV_GroupID
/// @param threadId SV_GroupThreadID
/// @param dispatchSize the size of the compute shader dispatch
CXX_CONSTEXPR uint getFrameBufferIndex(uint2 dispatchId, uint2 dispatchSize) {
    uint x = getFrameBufferX(dispatchId.x, dispatchSize.x);
    uint y = getFrameBufferY(dispatchId.y, dispatchSize.y);

    return (x * VIC20_SCREEN_WIDTH + y);
}

#ifdef __cplusplus
constexpr void test0() {
    static constexpr uint2 kScreenSize { VIC20_SCREEN_WIDTH, VIC20_SCREEN_HEIGHT };
    static constexpr uint2 kNumThreads { VIC20_THREADS_X, VIC20_THREADS_Y };
    static constexpr uint2 kDispatchSize = kScreenSize / kNumThreads - 1;

    auto SV_DispatchThreadId = [&](uint2 groupId, uint2 threadId) {
        return groupId * kNumThreads + threadId;
    };

    static_assert(getTextureCoords({ 0, 0 }, { 0, 0 }, kNumThreads) == uint2 { 0, 0 });
    static_assert(getTextureCoords(kDispatchSize, { VIC20_THREADS_X, VIC20_THREADS_Y }, kNumThreads) == kScreenSize);

    static_assert(getFrameBufferX(0, kDispatchSize.x + 1) == 0);
    // static_assert(getFrameBufferX(SV_DispatchThreadId(kDispatchSize.x / 2, kNumThreads / 2).x, kDispatchSize.x + 1) == VIC20_SCREEN_WIDTH / 2);
    static_assert(getFrameBufferX(SV_DispatchThreadId(kDispatchSize.x, VIC20_THREADS_X).x, kDispatchSize.x + 1) == VIC20_SCREEN_WIDTH);

    static_assert(getFrameBufferY(0, kDispatchSize.y + 1) == 0);
    static_assert(getFrameBufferY(SV_DispatchThreadId(kDispatchSize.y / 2, kNumThreads / 2).y, kDispatchSize.y + 1) == VIC20_SCREEN_HEIGHT / 2);
    static_assert(getFrameBufferY(SV_DispatchThreadId(kDispatchSize.y, kNumThreads).y, kDispatchSize.y + 1) == VIC20_SCREEN_HEIGHT);

    static_assert(getFrameBufferIndex(SV_DispatchThreadId({ 0, 0 }, { 0, 0 }), kDispatchSize) == 0);
    // static_assert(getFrameBufferIndex(SV_DispatchThreadId(kDispatchSize / 2, kNumThreads / 2), kDispatchSize + 1) == VIC20_FRAMEBUFFER_SIZE / 2);
    // static_assert(getFrameBufferIndex(SV_DispatchThreadId(kDispatchSize, kNumThreads), kDispatchSize + 1) == VIC20_FRAMEBUFFER_SIZE);
}
#endif

#ifndef __HLSL_VERSION
}
#endif
