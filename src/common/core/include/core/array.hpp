#pragma once

#include "core/unique.hpp"

namespace sm {
    template<typename T, size_t N>
    class Array {
        T m_data[N];
    public:
        constexpr T &operator[](size_t index) { verify_index(index); return m_data[index]; }
        constexpr const T &operator[](size_t index) const { verify_index(index); return m_data[index]; }

        constexpr T *begin() { return m_data; }
        constexpr const T *begin() const { return m_data; }

        constexpr T *end() { return m_data + N; }
        constexpr const T *end() const { return m_data + N; }

        constexpr size_t length() const { return N; }

        constexpr void fill(const T &value) {
            for (auto& it : *this) {
                it = value;
            }
        }

        constexpr void verify_index(size_t index) const {
            CTASSERTF(index < N, "Index out of bounds: %zu >= %zu", index, N);
        }
    };

    template<typename T, size_t N>
    constexpr auto to_array(const T (&data)[N]) {
        Array<T, N> result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = data[i];
        }
        return result;
    }

    template<typename T, typename TDelete = DefaultDelete<T[]>>
    class UniqueArray : public UniquePtr<T, TDelete> {
        using Super = UniquePtr<T, TDelete>;
        size_t m_length = 0;

    public:
        using Iterator = T *;
        using ConstIterator = const T *;

        constexpr UniqueArray() = default;

        constexpr UniqueArray(T *data, size_t capacity)
            : Super(data)
            , m_length(capacity)
        { }

        constexpr UniqueArray(size_t capacity)
            : UniqueArray(new T[capacity], capacity)
        { }

        constexpr UniqueArray(size_t capacity, T init)
            : UniqueArray(new T[capacity], capacity)
        {
            fill(init);
        }

        constexpr void resize(size_t capacity) {
            if (m_length >= capacity)
                return;

            Super::reset(new T[capacity]());
            m_length = capacity;
        }

        constexpr void resize(size_t capacity, T init) {
            if (m_length >= capacity)
                return;

            Super::reset(new T[capacity]);
            m_length = capacity;
            fill(init);
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
