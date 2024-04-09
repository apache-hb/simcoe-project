// shared data types between C++ and shader code
#include "common.hpp"

// configuration

#define DEPTH_BOUNDS_DISABLED 0
#define DEPTH_BOUNDS_ENABLED 1
#define DEPTH_BOUNDS_MSAA 2

#ifndef DEPTH_BOUNDS_MODE
#   define DEPTH_BOUNDS_MODE DEPTH_BOUNDS_ENABLED
#endif

#define LIGHT_CULLING_DISABLED 0
#define LIGHT_CULLING_ENABLED 1

#ifndef LIGHT_CULLING_MODE
#   define LIGHT_CULLING_MODE LIGHT_CULLING_ENABLED
#endif

#define ALPHA_TEST_DISABLED 0
#define ALPHA_TEST_ENABLED 1

#ifndef ALPHA_TEST_MODE
#   define ALPHA_TEST_MODE ALPHA_TEST_DISABLED
#endif

#ifndef MAX_LIGHTS_PER_TILE
#   define MAX_LIGHTS_PER_TILE 256
#endif

#ifndef MAX_POINT_LIGHTS_PER_TILE
#   define MAX_POINT_LIGHTS_PER_TILE (MAX_LIGHTS_PER_TILE / 2)
#endif

#ifndef MAX_SPOT_LIGHTS_PER_TILE
#   define MAX_SPOT_LIGHTS_PER_TILE  (MAX_LIGHTS_PER_TILE / 2)
#endif

#ifndef TILE_SIZE
#   define TILE_SIZE 16
#endif

#define FLOAT_MAX 3.402823466e+38f

// TODO: get this working
// * use uint16_t for light indices, counts, window size, depth texture size, etc
// * test everything

cbuffer FrameBuffer : register(b0) {
    ViewportData gCameraData;
};

uint get_tile_index(float2 screen) {
    float res = float(TILE_SIZE);
    uint cells_x = (gCameraData.window_width() + TILE_SIZE - 1) / TILE_SIZE;
    uint tile_idx = floor(screen.x / res) + floor(screen.y / res) * cells_x;

    return tile_idx;
}
