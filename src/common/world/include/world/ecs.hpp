#pragma once

#include "math/math.hpp"

namespace sm::world {
    struct LightTag {};
    struct PointLightTag {};
    struct SpotLightTag {};

    struct Colour { math::float3 colour; };
    struct Direction { math::quatf direction; };
    struct Intensity { float intensity; };

    struct Position { math::float3 position; };
    struct Rotation { math::quatf rotation; };
    struct Scale { math::float3 scale; };
}
