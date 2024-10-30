#pragma once

#ifndef __HLSL_VERSION
#include "math/math.hpp"
namespace sm::draw::shared { using namespace sm::math;
#endif

struct Vic20Info {
    uint2 textureSize;
};

#define VIC20_THREADS_X 8
#define VIC20_THREADS_Y 8

#define VIC20_PALETTE_SIZE 16
#define VIC20_SCREEN_WIDTH 176
#define VIC20_SCREEN_HEIGHT 184
#define VIC20_SCREEN_SIZE (VIC20_SCREEN_WIDTH * VIC20_SCREEN_HEIGHT)
#define VIC20_BPP 4
#define VIC20_FRAMEBUFFER_SIZE ((VIC20_SCREEN_SIZE * VIC20_BPP) / 8)

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

#define VIC20_PALETTE_MASK (VIC20_PALETTE_SIZE - 1)

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

#ifndef __HLSL_VERSION
}
#endif