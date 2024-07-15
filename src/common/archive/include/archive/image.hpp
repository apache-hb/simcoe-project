#pragma once

#include "core/adt/vector.hpp"
#include "core/fs.hpp"

#include <expected>
#include <span>

#include "math/math.hpp"

#include "format.reflect.h"

namespace sm {
    struct ImageData {
        Format pxformat;
        math::uint2 size;
        sm::Vector<byte> data;

        size_t sizeInBytes() const {
            return data.size();
        }

        math::uint2 size2d() const {
            return size;
        }
    };

    std::expected<ImageData, std::string> loadImage(std::span<const byte> data);
    std::expected<ImageData, std::string> openImage(const fs::path& path);
}
