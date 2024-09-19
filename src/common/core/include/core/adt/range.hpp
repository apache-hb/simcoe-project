#pragma once

#include "core/core.hpp"
#include "base/panic.h"

#include <algorithm>

namespace sm::core::detail {
    template<typename T>
    class Collection {
    protected:
        T *mFront;
        T *mBack;

        constexpr Collection(T *begin, T *end) noexcept
            : mFront(begin)
            , mBack(end)
        { }

        constexpr T *getFront() noexcept requires (!std::is_const_v<T>) { return mFront; }
        constexpr T *getBack() noexcept requires (!std::is_const_v<T>) { return mBack; }

        constexpr const T *getFront() const noexcept { return mFront; }
        constexpr const T *getBack() const noexcept { return mBack; }

        constexpr void verifyIndex(size_t index) const noexcept {
            CTASSERTF(index < size(), "index out of bounds (%zu >= %zu)", index, size());
        }

        constexpr void verifyIndex(ssize_t index) const noexcept {
            CTASSERTF(index >= 0, "index is negative (%zd)", index);
            CTASSERTF(index < ssize(), "index out of bounds (%zd >= %zd)", index, ssize());
        }

    public:
        constexpr size_t size() const noexcept { return mBack - mFront; }
        constexpr ssize_t ssize() const noexcept { return mBack - mFront; }

        constexpr size_t sizeInBytes() const noexcept { return size() * sizeof(T); }
        constexpr ssize_t ssizeInBytes() const noexcept { return ssize() * sizeof(T); }

        constexpr bool empty() const noexcept { return mFront == mBack; }
        constexpr bool isEmpty() const noexcept { return mFront == mBack; }

        constexpr T *begin() noexcept requires (!std::is_const_v<T>) { return mFront; }
        constexpr T *end() noexcept requires (!std::is_const_v<T>) { return mBack; }

        constexpr const T *begin() const noexcept { return mFront; }
        constexpr const T *end() const noexcept { return mBack; }

        constexpr const T *cbegin() const noexcept { return mFront; }
        constexpr const T *cend() const noexcept { return mBack; }

        constexpr T *data() noexcept requires (!std::is_const_v<T>) { return mFront; }
        constexpr const T *data() const noexcept { return mFront; }

        constexpr T& operator[](size_t index) noexcept requires (!std::is_const_v<T>) {
            verifyIndex(index);
            return mFront[index];
        }

        constexpr const T& operator[](size_t index) const noexcept {
            verifyIndex(index);
            return mFront[index];
        }

        constexpr T& at(size_t index) noexcept requires (!std::is_const_v<T>) {
            verifyIndex(index);
            return mFront[index];
        }

        constexpr const T& at(size_t index) const noexcept {
            verifyIndex(index);
            return mFront[index];
        }

        constexpr bool contains(const T& value) const noexcept {
            return std::find(begin(), end(), value) != end();
        }
    };
}
