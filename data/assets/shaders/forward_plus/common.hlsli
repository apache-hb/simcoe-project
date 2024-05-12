// shared data types between C++ and shader code
#include "common.hpp"

// configuration

#ifndef DEPTH_BOUNDS_MODE
#   define DEPTH_BOUNDS_MODE DEPTH_BOUNDS_DISABLED
#endif

#ifndef ALPHA_TEST_MODE
#   define ALPHA_TEST_MODE ALPHA_TEST_DISABLED
#endif

#define FLOAT_MAX 3.402823466e+38f

// TODO: get this working
// * use uint16_t for light indices, counts, window size, depth texture size, etc
// * test everything

cbuffer FrameBuffer : register(b0) {
    ViewportData gCameraData;
};
