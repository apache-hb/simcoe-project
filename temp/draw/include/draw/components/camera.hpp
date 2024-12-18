#pragma once

#include "math/vector.hpp"

namespace sm::draw::next {
    struct ViewportInfo {
        DXGI_FORMAT colour;
        DXGI_FORMAT depth;
        math::uint2 size;
    };

    struct CameraInfo {
        math::float3 position;
        math::quatf direction;
        math::radf fov;
    };
}
