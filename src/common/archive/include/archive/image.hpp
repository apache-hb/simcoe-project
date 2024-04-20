#pragma once

#include "core/span.hpp"
#include "core/vector.hpp"
#include "core/fs.hpp"

#include "math/math.hpp"

#include "format.reflect.h"

namespace sm {
    struct ImageData {
        Format pxformat;
        math::uint2 size;
        sm::Vector<byte> data;

        bool is_valid() const { return !data.empty(); }
    };

    ImageData load_image(sm::Span<const uint8> data);
    ImageData open_image(const fs::path& path);
}
