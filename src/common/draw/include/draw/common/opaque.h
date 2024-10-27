#pragma once

#ifndef __HLSL_VERSION
#   include "math/math.hpp"
namespace sm::draw::shared { using namespace sm::math;
#endif

struct CameraInfo {
    float4x4 mvp;
};

#ifndef __HLSL_VERSION
} // namespace sm::draw::shared
#endif
