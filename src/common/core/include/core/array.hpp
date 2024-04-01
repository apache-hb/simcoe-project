#pragma once

#include "core/unique.hpp"

namespace sm {
    template<typename T, size_t N>
    class Array {
        T mData[N];
    public:
        constexpr T &operator[](size_t index) { verify_index(index); return mData[index]; }
        constexpr const T &operator[](size_t index) const { verify_index(index); return mData[index]; }

        constexpr T *begin() { return mData; }
        constexpr const T *begin() const { return mData; }

        constexpr T *end() { return mData + N; }
        constexpr const T *end() const { return mData + N; }

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
        size_t mLength = 0;

    public:
        using Iterator = T *;
        using ConstIterator = const T *;

        constexpr UniqueArray() = default;

        constexpr UniqueArray(T *data, size_t capacity)
            : Super(data)
            , mLength(capacity)
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
            Super::reset(new T[capacity]());
            mLength = capacity;
        }

        constexpr void resize(size_t capacity, T init) {
            Super::reset(new T[capacity]);
            mLength = capacity;
            fill(init);
        }

        constexpr size_t length() const { return mLength; }

        constexpr T &operator[](size_t index) { return Super::get()[index]; }
        constexpr const T &operator[](size_t index) const { return Super::get()[index]; }

        constexpr T *begin() { return Super::get(); }
        constexpr const T *begin() const { return Super::get(); }

        constexpr T *end() { return Super::get() + mLength; }
        constexpr const T *end() const { return Super::get() + mLength; }

        constexpr void fill(const T &value) {
            for (auto& it : *this) {
                it = value;
            }
        }
    };
}
