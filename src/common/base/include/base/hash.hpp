#pragma once

#include <type_traits> // IWYU pragma: export

namespace sm {
    template<typename T>
    void hash_combine(size_t &seed, const T &value) {
        seed ^= std::hash<T>{}(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    template<typename... T>
    void hash_combine(size_t &seed, const T &... values) {
        (hash_combine(seed, values), ...);
    }
}
