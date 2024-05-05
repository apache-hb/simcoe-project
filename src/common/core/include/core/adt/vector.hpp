#pragma once

#include "core/core.hpp"

#include "base/panic.h"

#include <vector>
#include <memory>
#include <algorithm>

namespace sm {
    template<typename T>
    using Vector = std::vector<T>;

    template<typename T, size_t N>
    using SmallVector = Vector<T>; // TODO: make a small vector

    template<typename T> requires (std::is_object_v<T>)
    class VectorBase final {
        using SizeType = ssize_t;
        using Self = VectorBase;

        T *mFront = nullptr;
        T *mBack = nullptr;
        T *mCapacity = nullptr;

        // destroy the data and release the memory
        constexpr void releaseData() noexcept {
            std::destroy(mFront, mBack);
            delete[] mFront;
        }

        // set new data pointers
        constexpr void updateData(T *front, SizeType used, SizeType capacity) noexcept {
            mFront = front;
            mBack = front + used;
            mCapacity = front + capacity;
        }

        constexpr void replace_data(T *front, SizeType used, SizeType capacity) noexcept {
            releaseData();
            updateData(front, used, capacity);
        }

        constexpr void init(SizeType capacity, SizeType used = 0) noexcept {
            CTASSERTF(capacity >= 0, "Capacity must be non-negative: %zd", capacity);
            CTASSERTF(used >= 0, "Used must be non-negative: %zd", used);
            CTASSERTF(used <= capacity, "Used must be less than or equal to capacity: %zd <= %zd", used, capacity);

            T *data = new T[capacity];

            updateData(data, used, capacity);
        }

        constexpr void verifyIndex(SizeType index) const noexcept {
            CTASSERTF(index >= 0, "Index must be non-negative: %zd", index);
            CTASSERTF(index < ssize(), "Index must be less than size: %zd < %zu", index, ssize());
        }

        // only ever grows the backing data
        constexpr void ensureGrowth(SizeType size) noexcept {
            if (size > capacity()) {
                SizeType new_capacity = (std::max)(capacity() * 2, size);
                SizeType old_size = ssize();

                // copy over the old data
                T *new_data = new T[new_capacity];
                std::move(begin(), end(), new_data);

                // release old data and update the pointers
                replace_data(new_data, old_size, new_capacity);
            }
        }

        // ensure extra space is available
        constexpr void ensureExtra(SizeType extra) noexcept {
            ensureGrowth(ssize() + extra);
        }

        // shrink the backing data, dont release memory
        constexpr void shrinkSize(SizeType size) noexcept {
            CTASSERTF(size >= 0, "Size must be non-negative: %zd", size);

            if (size < ssize()) {
                std::destroy(mFront + size, mBack);
                mBack = mFront + size;
            }
        }

        // shrink the backing data and release extra memory
        constexpr void shrinkCapacity(SizeType size) noexcept {
            CTASSERTF(size >= 0, "Size must be non-negative: %zd", size);

            if (size < capacity()) {
                SizeType old_size = ssize();

                SizeType count = (std::min)(old_size, size);

                // copy over the old data
                T *new_data = new T[size];
                std::move(begin(), begin() + count, new_data);

                // release old data and update the pointers
                replace_data(new_data, count, size);
            }
        }

        constexpr void setCapacity(SizeType cap) noexcept {
            CTASSERTF(cap >= 0, "Capacity must be non-negative: %zd", cap);

            if (cap != capacity()) {
                SizeType old_size = ssize();

                // number of elements to copy
                SizeType count = (std::min)(old_size, cap);

                // copy over the old data
                T *new_data = new T[cap];
                std::move(begin(), begin() + count, new_data);

                // release old data and update the pointers
                replace_data(new_data, count, cap);
            }
        }

    public:
        constexpr ~VectorBase() noexcept {
            delete[] mFront;
        }

        constexpr VectorBase() noexcept {
            init(4);
        }

        constexpr VectorBase(SizeType initial) noexcept {
            init(initial, initial);
        }

        explicit constexpr VectorBase(const VectorBase &other) noexcept {
            init(other.ssize(), other.ssize());
            std::move(other.begin(), other.end(), begin());
        }

        constexpr VectorBase(VectorBase &&other) noexcept {
            swap(*this, other);
        }

        // delete the copy assignment operator since it cant be made
        // explicit. use clone() instead.
        VectorBase& operator=(const VectorBase &other) noexcept = delete;

        constexpr VectorBase& operator=(VectorBase &&other) noexcept {
            swap(*this, other);
            return *this;
        }

        constexpr VectorBase clone() const noexcept {
            return Self(*this);
        }

        constexpr VectorBase(const T *first, const T *last) noexcept {
            init(last - first);
            std::move(first, last, begin());
        }

        template<size_t N>
        constexpr VectorBase(const T (&array)[N]) noexcept {
            init(N);
            std::move(array, array + N, begin());
        }

        // size query

        constexpr size_t size() const noexcept { return mBack - mFront; }
        constexpr size_t sizeInBytes() const noexcept { return ssize() * sizeof(T); }

        constexpr ssize_t ssize() const noexcept { return mBack - mFront; }
        constexpr ssize_t ssizeInBytes() const noexcept { return ssize() * sizeof(T); }

        constexpr ssize_t capacity() const noexcept { return mCapacity - mFront; }
        constexpr bool empty() const noexcept { return mFront == mBack; }

        // element access

        constexpr T& operator[](SizeType index) noexcept {
            verifyIndex(index);
            return mFront[index];
        }

        constexpr const T& operator[](SizeType index) const noexcept {
            verifyIndex(index);
            return mFront[index];
        }

        constexpr T& at(SizeType index) noexcept {
            verifyIndex(index);
            return mFront[index];
        }

        constexpr const T& at(SizeType index) const noexcept {
            verifyIndex(index);
            return mFront[index];
        }

        constexpr T *data() noexcept { return mFront; }
        constexpr const T *data() const noexcept { return mFront; }

        constexpr T *release() noexcept {
            T *data = mFront;
            updateData(nullptr, 0, 0);
            return data;
        }

        // iteration

        constexpr T *begin() noexcept { return mFront; }
        constexpr const T *begin() const noexcept { return mFront; }

        constexpr T *end() noexcept { return mBack; }
        constexpr const T *end() const noexcept { return mBack; }

        // resizing

        constexpr void clear() noexcept {
            std::destroy(mFront, mBack);
            mBack = mFront;
        }

        // reserve space for at least size elements
        constexpr void reserve(SizeType count) noexcept {
            CTASSERTF(count >= 0, "Size must be non-negative: %zd", count);
            ensure_size(count);
        }

        // reserve space for exactly size elements
        constexpr void reserve_exact(SizeType count) noexcept {
            CTASSERTF(count >= 0, "Size must be non-negative: %zd", count);
            ensure_capacity(count);
        }

        // only change the size, dont change the capacity
        constexpr void resize(SizeType count) noexcept {
            CTASSERTF(count >= 0, "Size must be non-negative: %zd", count);

            if (count > ssize()) {
                ensureGrowth(count);
                std::uninitialized_value_construct(mBack, mFront + count);
            } else if (count < ssize()) {
                shrinkSize(count);
            }

            mBack = mFront + count;
        }

        // change the size and capacity
        constexpr void resize_exact(SizeType count) noexcept {
            CTASSERTF(count >= 0, "Size must be non-negative: %zd", count);

            if (count > ssize()) {
                ensureGrowth(count);
                std::uninitialized_value_construct(mBack, mFront + count);
            } else if (count < ssize()) {
                shrinkCapacity(count);
            }

            mBack = mFront + count;
        }

        // appending

        constexpr T& emplace_back(auto&&... args) noexcept {
            ensureExtra(1);
            return *new (mBack++) T(std::forward<decltype(args)>(args)...);
        }

        constexpr T& emplace_back(T &&value) noexcept {
            ensureExtra(1);
            return (*mBack++ = std::move(value));
        }

        constexpr void push_back(const T &value) noexcept {
            ensureExtra(1);
            *mBack++ = value;
        }

        constexpr void push_back(T &&value) noexcept {
            ensureExtra(1);
            *mBack++ = std::move(value);
        }

        constexpr void assign(const T *first, const T *last) noexcept {
            SizeType count = last - first;
            ensureExtra(count);
            std::move(first, last, mBack);
            mBack += count;
        }

        // swap

        friend void swap(VectorBase& lhs, VectorBase& rhs) noexcept {
            std::swap(lhs.mFront, rhs.mFront);
            std::swap(lhs.mBack, rhs.mBack);
            std::swap(lhs.mCapacity, rhs.mCapacity);
        }
    };

    template<typename T, size_t N>
    class SmallVectorBase {
        T mStorage[N];
    };
}
