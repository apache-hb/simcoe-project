#pragma once

#include <vector>

namespace sm {
    template<typename T>
    using Vector = std::vector<T>;

    template<typename T, size_t N>
    using SmallVector = Vector<T>; // TODO: make a small vector
}
