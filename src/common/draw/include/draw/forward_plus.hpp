#pragma once

#include "draw/common.hpp"

namespace sm::draw {
    static constexpr inline size_t kMaxViewports = MAX_VIEWPORTS;
    static constexpr inline size_t kMaxObjects = MAX_OBJECTS;
    static constexpr inline size_t kMaxLights = MAX_LIGHTS;
    static constexpr inline size_t kMaxPointLights = MAX_POINT_LIGHTS;
    static constexpr inline size_t kMaxSpotLights = MAX_SPOT_LIGHTS;

    enum class DepthBoundsMode {
        eDisabled = DEPTH_BOUNDS_DISABLED,
        eEnabled = DEPTH_BOUNDS_ENABLED,
        eEnabledMSAA = DEPTH_BOUNDS_MSAA,
    };

    enum class AlphaTestMode {
        eDisabled = ALPHA_TEST_DISABLED,
        eEnabled = ALPHA_TEST_ENABLED,
    };

    struct ForwardPlusConfig {
        DepthBoundsMode depthBounds;
        AlphaTestMode alphaTest;
    };
}
