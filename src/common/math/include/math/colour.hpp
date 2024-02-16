#pragma once

#include "math.hpp"

namespace sm::math {
    constexpr uint32_t pack_colour(const float4& colour) {
        return (uint32_t(colour.r * 255.0f) << 24) |
               (uint32_t(colour.g * 255.0f) << 16) |
               (uint32_t(colour.b * 255.0f) << 8) |
               (uint32_t(colour.a * 255.0f));
    }
}
