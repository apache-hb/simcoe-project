#pragma once

#include "math/math.hpp"
#include "core/vector.hpp"

#include "format.reflect.h"

namespace sm {
    struct ImageData {
        math::uint2 size;
        Format format;
        sm::Vector<byte> data;
    };
}
