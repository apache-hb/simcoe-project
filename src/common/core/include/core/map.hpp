#pragma once

#include <map>
#include <unordered_map>

namespace sm {
    template<typename K, typename V>
    using Map = std::map<K, V, std::less<K>>;

    template<typename K, typename V>
    using MultiMap = std::multimap<K, V, std::less<K>>;

    template<typename K, typename V, typename H = std::hash<K>, typename EQ = std::equal_to<K>>
    using HashMap = std::unordered_map<K, V, H, EQ>;

    template<typename K, typename V, typename H = std::hash<K>, typename EQ = std::equal_to<K>>
    using MultiHashMap = std::unordered_multimap<K, V, H, EQ>;
}
