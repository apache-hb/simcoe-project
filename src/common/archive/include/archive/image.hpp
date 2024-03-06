#pragma once

#include "core/span.hpp"
#include "core/vector.hpp"
#include "core/fs.hpp"

#include "math/math.hpp"

#include "format.reflect.h"

namespace sm {
    struct ImageData {
        ImageFormat format;
        Format pxformat;
        math::uint2 size;
        sm::Vector<byte> data;
    };

    ImageData load_image(sm::Span<const uint8> data);
    ImageData open_image(const fs::path& path);
}
