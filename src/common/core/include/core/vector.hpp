#pragma once

#include "core/arena.hpp"
#include "core/traits.hpp"

#include <vector>

namespace sm {
    template<typename T>
    using Vector = std::vector<T, sm::StandardArena<T>>;

    template<typename T, size_t N>
    class StaticVector {
        T m_data[N];
        size_t m_used = 0;

        constexpr void verify_index(size_t index) const {
            SM_ASSERTF(index < N, "index out of bounds (%zu < %zu)", index, N);
        }

    public:
        constexpr StaticVector() = default;

        constexpr size_t size() const { return m_used; }
        constexpr size_t capacity() const { return N; }

        constexpr T *data() { return m_data; }
        constexpr const T *data() const { return m_data; }

        constexpr T &operator[](size_t index) { verify_index(index); return m_data[index]; }
        constexpr const T &operator[](size_t index) const { verify_index(index); return m_data[index]; }

        constexpr T *begin() { return m_data; }
        constexpr const T *begin() const { return m_data; }

        constexpr T *end() { return m_data + m_used; }
        constexpr const T *end() const { return m_data + m_used; }

        constexpr T& emplace_back(auto&&... args) {
            SM_ASSERTF(m_used < N, "vector overflow");
            return m_data[m_used++] = T{std::forward<decltype(args)>(args)...};
        }
    };
}
