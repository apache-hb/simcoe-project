#pragma once

#include "core/vector.hpp"

#include <queue>

namespace sm {
    template<typename T, typename C = sm::Vector<T>, typename Compare = std::less<T>>
    using PriorityQueue = std::priority_queue<T, C, Compare>;
}
