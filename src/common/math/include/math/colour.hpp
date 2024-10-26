#pragma once

#include "math.hpp"

namespace sm::math {
    constexpr uint32 pack_colour(const float4& colour) {
        return (uint32_t(colour.r * 255.0f) << 24) |
               (uint32_t(colour.g * 255.0f) << 16) |
               (uint32_t(colour.b * 255.0f) << 8) |
               (uint32_t(colour.a * 255.0f));
    }

    constexpr float4 unpack_colour(uint32 colour) {
        return {
            float((colour >> 24) & 0xFF) / 255.0f,
            float((colour >> 16) & 0xFF) / 255.0f,
            float((colour >> 8) & 0xFF) / 255.0f,
            float(colour & 0xFF) / 255.0f
        };
    }

    static constexpr float4 kColourClear = {0.0f, 0.0f, 0.0f, 0.0f};
    static constexpr float4 kColourBlack = {0.0f, 0.0f, 0.0f, 1.0f};
    static constexpr float4 kColourWhite = {1.0f, 1.0f, 1.0f, 1.0f};
    static constexpr float4 kColourRed   = {1.0f, 0.0f, 0.0f, 1.0f};
    static constexpr float4 kColourGreen = {0.0f, 1.0f, 0.0f, 1.0f};
    static constexpr float4 kColourBlue  = {0.0f, 0.0f, 1.0f, 1.0f};
}
