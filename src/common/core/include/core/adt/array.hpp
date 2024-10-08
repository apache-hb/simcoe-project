#pragma once

#include "core/memory/unique.hpp"

namespace sm {
    template<typename T, size_t N>
    class Array {
        T mData[N];

        constexpr void verifyIndex(size_t index) const noexcept {
            CTASSERTF(index < N, "Index out of bounds: %zu >= %zu", index, N);
        }
    public:
        constexpr T &operator[](size_t index) noexcept { verifyIndex(index); return mData[index]; }
        constexpr const T &operator[](size_t index) const noexcept { verifyIndex(index); return mData[index]; }

        constexpr T *begin() noexcept { return mData; }
        constexpr const T *begin() const noexcept { return mData; }

        constexpr T *end() noexcept { return mData + N; }
        constexpr const T *end() const noexcept { return mData + N; }

        constexpr size_t length() const noexcept { return N; }

        constexpr void fill(const T &value) noexcept {
            for (auto& it : *this) {
                it = value;
            }
        }
    };

    template<typename T, size_t N>
    constexpr Array<T, N> toArray(const T (&data)[N]) noexcept {
        Array<T, N> result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = data[i];
        }
        return result;
    }

    template<typename T, typename TDelete = DefaultDelete<T[]>>
    class UniqueArray : public UniquePtr<T, TDelete> {
        using Super = UniquePtr<T, TDelete>;
        ssize_t mLength;

    public:
        using Iterator = T *;
        using ConstIterator = const T *;

        constexpr UniqueArray() = default;

        constexpr UniqueArray(T *data, size_t capacity) noexcept
            : Super(data)
            , mLength(capacity)
        { }

        constexpr UniqueArray(size_t capacity) noexcept
            : UniqueArray(new T[capacity], capacity)
        { }

        constexpr UniqueArray(size_t capacity, T init) noexcept
            : UniqueArray(new T[capacity], capacity)
        {
            fill(init);
        }

        constexpr void resize(size_t capacity) noexcept {
            Super::reset(new T[capacity]());
            mLength = capacity;
        }

        constexpr void resize(size_t capacity, T init) noexcept {
            Super::reset(new T[capacity]);
            mLength = capacity;
            fill(init);
        }

        constexpr size_t size() const noexcept { return mLength; }
        constexpr size_t sizeInBytes() const noexcept { return mLength * sizeof(T); }

        constexpr ssize_t ssize() const noexcept { return mLength; }
        constexpr ssize_t ssizeInBytes() const noexcept { return mLength * sizeof(T); }

        constexpr T &operator[](size_t index) noexcept { return Super::get()[index]; }
        constexpr const T &operator[](size_t index) const noexcept { return Super::get()[index]; }

        constexpr T *begin() noexcept { return Super::get(); }
        constexpr const T *begin() const noexcept { return Super::get(); }

        constexpr T *end() noexcept { return Super::get() + mLength; }
        constexpr const T *end() const noexcept { return Super::get() + mLength; }

        constexpr void fill(const T &value) noexcept {
            for (auto& it : *this) {
                it = value;
            }
        }
    };

    template<typename T, typename TDelete = DefaultDelete<T[]>>
    constexpr UniqueArray<T, TDelete> makeUniqueArray(size_t capacity) noexcept {
        return UniqueArray<T, TDelete>(capacity);
    }
}
