#pragma once

#include "core/arena.hpp"

#include <set>

namespace sm {
    template<typename T>
    using Set = std::set<T, std::less<T>, sm::StandardArena<T>>;
}
