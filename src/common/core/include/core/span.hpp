#pragma once

#include <span>

namespace sm {
    template<typename T, size_t N = std::dynamic_extent>
    using Span = std::span<T, N>;

    template<typename T, size_t N = std::dynamic_extent>
    using View = std::span<const T, N>;
}
