#pragma once

#include "core/arena.hpp"

#include <vector>

namespace sm {
    template<typename T>
    using Vector = std::vector<T, sm::StandardArena<T>>;
}
