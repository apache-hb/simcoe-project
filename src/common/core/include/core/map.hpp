#pragma once

#include "core/arena.hpp"

#include <map>
#include <unordered_map>

namespace sm {
    template<typename K, typename V>
    using Map = std::map<K, V, std::less<K>, sm::StandardArena<std::pair<const K, V>>>;

    template<typename K, typename V>
    using MultiMap = std::multimap<K, V, std::less<K>, sm::StandardArena<std::pair<const K, V>>>;

    template<typename K, typename V, typename H = std::hash<K>, typename EQ = std::equal_to<K>>
    using HashMap = std::unordered_map<K, V, H, EQ, sm::StandardArena<std::pair<const K, V>>>;

    template<typename K, typename V, typename H = std::hash<K>, typename EQ = std::equal_to<K>>
    using MultiHashMap = std::unordered_multimap<K, V, H, EQ, sm::StandardArena<std::pair<const K, V>>>;
}
