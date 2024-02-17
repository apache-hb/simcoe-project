#pragma once

#include <set>

namespace sm {
    template<typename T>
    using Set = std::set<T, std::less<T>>;
}
