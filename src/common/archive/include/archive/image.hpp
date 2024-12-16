#pragma once

#include "core/adt/vector.hpp"
#include "base/fs.hpp"

#include <expected>
#include <span>

#include "math/math.hpp"

namespace sm {
    enum class Format : uint16_t {
        unknown,

        rg32float,
        rgb32float,
        rgba32float,

        r8byte,
        rg8byte,
        rgb8byte,
        rgba8byte,

        uint16,
        uint32,
    };

    std::string_view toString(Format format);

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
