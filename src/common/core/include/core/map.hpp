#pragma once

#include "core/arena.hpp"
#include <map>

namespace sm {
    template<typename K, typename V>
    using Map = std::map<K, V, std::less<K>, sm::StandardArena<std::pair<const K, V>>>;

    template<typename K, typename V>
    using MultiMap = std::multimap<K, V, std::less<K>, sm::StandardArena<std::pair<const K, V>>>;
}
