#pragma once

#include "core/arena.hpp"
#include "core/unique.hpp"

#include <vector>

namespace sm {
    template<typename T>
    using Vector = std::vector<T, sm::StandardArena<T>>;

    template<typename T, typename TDelete = DefaultDelete<T>>
    class UniqueArray : public UniquePtr<T, TDelete> {
        using Super = UniquePtr<T, TDelete>;
        size_t m_length = 0;

    public:
        constexpr UniqueArray() = default;

        constexpr UniqueArray(T *data, size_t capacity)
            : Super(data)
            , m_length(capacity)
        { }

        constexpr UniqueArray(size_t capacity)
            : UniqueArray(new T[capacity], capacity)
        { }

        constexpr void resize(size_t capacity) {
            if (m_length >= capacity)
                return;

            Super::reset(new T[capacity]);
            m_length = capacity;
        }

        constexpr size_t length() const { return m_length; }

        constexpr T &operator[](size_t index) { return Super::get()[index]; }
        constexpr const T &operator[](size_t index) const { return Super::get()[index]; }

        constexpr T *begin() { return Super::get(); }
        constexpr const T *begin() const { return Super::get(); }

        constexpr T *end() { return Super::get() + m_length; }
        constexpr const T *end() const { return Super::get() + m_length; }

        constexpr void fill(const T &value) {
            for (auto& it : *this) {
                it = value;
            }
        }
    };
}
