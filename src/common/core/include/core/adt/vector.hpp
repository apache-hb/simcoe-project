#pragma once

#include "core/adt/range.hpp"
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

    template<typename T> requires (std::is_object_v<T> && std::is_move_constructible_v<T>)
    class VectorBase final : public detail::Collection<T> {
        using Super = detail::Collection<T>;

        using SizeType = ssize_t;
        using Self = VectorBase;

        T *mCapacity = nullptr;

        // destroy the data and release the memory
        constexpr void releaseData() noexcept {
            std::destroy(this->mFront, this->mBack);
            delete[] this->mFront;
        }

        // set new data pointers
        constexpr void updateData(T *front, SizeType used, SizeType capacity) noexcept {
            this->mFront = front;
            this->mBack = front + used;
            mCapacity = front + capacity;
        }

        constexpr void replaceData(T *front, SizeType used, SizeType capacity) noexcept {
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

        // only ever grows the backing data
        constexpr void ensureGrowth(SizeType size) noexcept {
            if (size > capacity()) {
                SizeType newCapacity = (std::max)(capacity() * 2, size);
                SizeType oldSize = this->ssize();

                // copy over the old data
                T *newData = new T[newCapacity];
                std::uninitialized_move(this->begin(), this->end(), newData);

                // release old data and update the pointers
                replaceData(newData, oldSize, newCapacity);
            }
        }

        // ensure the internal capacity is exactly size
        constexpr void ensureCapacity(SizeType size) noexcept {
            CTASSERTF(size >= 0, "Size must be non-negative: %zd", size);

            if (size != capacity()) {
                SizeType oldSize = this->ssize();

                // copy over the old data
                T *newData = new T[size];
                std::uninitialized_move(this->begin(), this->begin() + oldSize, newData);

                // release old data and update the pointers
                replaceData(newData, oldSize, size);
            }
        }

        // ensure extra space is available
        constexpr void ensureExtra(SizeType extra) noexcept {
            ensureGrowth(this->ssize() + extra);
        }

        // shrink the backing data, dont release memory
        constexpr void shrinkSize(SizeType size) noexcept {
            CTASSERTF(size >= 0, "Size must be non-negative: %zd", size);

            if (size < this->ssize()) {
                std::destroy(this->mFront + size, this->mBack);
                this->mBack = this->mFront + size;
            }
        }

        // shrink the backing data and release extra memory
        constexpr void shrinkCapacity(SizeType size) noexcept {
            CTASSERTF(size >= 0, "Size must be non-negative: %zd", size);

            if (size < capacity()) {
                SizeType oldSize = this->ssize();

                SizeType count = (std::min)(oldSize, size);

                // copy over the old data
                T *newData = new T[size];
                std::uninitialized_move(this->begin(), this->begin() + count, newData);

                // release old data and update the pointers
                replaceData(newData, count, size);
            }
        }

        constexpr void setCapacity(SizeType cap) noexcept {
            CTASSERTF(cap >= 0, "Capacity must be non-negative: %zd", cap);

            if (cap != capacity()) {
                SizeType oldSize = this->ssize();

                // number of elements to copy
                SizeType count = (std::min)(oldSize, cap);

                // copy over the old data
                T *newData = new T[cap];
                std::uninitialized_move(this->begin(), this->begin() + count, newData);

                // release old data and update the pointers
                replaceData(newData, count, cap);
            }
        }

        constexpr VectorBase(T *front, T *back, T *capacity) noexcept
            : Super(front, back)
            , mCapacity(capacity)
        { }

        constexpr VectorBase(SizeType initial, SizeType used)
            : Super(nullptr, nullptr)
        {
            init(initial, used);
        }

    public:
        constexpr ~VectorBase() noexcept {
            delete[] this->mFront;
        }

        constexpr VectorBase(SizeType initial) noexcept
            : VectorBase(initial, 0)
        { }

        constexpr VectorBase() noexcept
            : VectorBase(4)
        { }

        explicit constexpr VectorBase(const VectorBase &other) noexcept
            : VectorBase(other.size(), other.size())
        {
            std::uninitialized_copy(other.begin(), other.end(), this->begin());
        }

        constexpr VectorBase(VectorBase &&other) noexcept
            : Super(nullptr, nullptr)
            , mCapacity(nullptr)
        {
            swap(*this, other);
        }

        constexpr VectorBase(std::initializer_list<T> init) noexcept
            : VectorBase(init.begin(), init.end())
        { }

        static constexpr VectorBase consume(T *data, SizeType size, SizeType capacity) noexcept {
            return VectorBase{data, data + size, data + capacity};
        }

        static constexpr VectorBase consume(T *data, SizeType size) noexcept {
            return consume(data, size, size);
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

        constexpr VectorBase(const T *first, const T *last) noexcept
            : VectorBase(last - first, last - first)
        {
            std::uninitialized_copy(first, last, this->begin());
        }

        template<size_t N>
        constexpr VectorBase(const T (&array)[N]) noexcept
            : VectorBase(array, array + N)
        { }

        // size query

        constexpr ssize_t capacity() const noexcept { return mCapacity - this->mFront; }

        // element access

        constexpr T *release() noexcept {
            T *data = this->mFront;
            updateData(nullptr, 0, 0);
            return data;
        }

        // resizing

        constexpr void clear() noexcept {
            std::destroy(this->mFront, this->mBack);
            this->mBack = this->mFront;
        }

        // reserve space for at least size elements
        constexpr void reserve(SizeType count) noexcept {
            CTASSERTF(count >= 0, "Size must be non-negative: %zd", count);
            ensureGrowth(count);
        }

        // reserve space for exactly size elements
        constexpr void reserve_exact(SizeType count) noexcept {
            CTASSERTF(count >= 0, "Size must be non-negative: %zd", count);
            ensureCapacity(count);
        }

        // only change the size, dont change the capacity
        constexpr void resize(SizeType count) noexcept {
            CTASSERTF(count >= 0, "Size must be non-negative: %zd", count);

            if (count > this->ssize()) {
                ensureGrowth(count);
                std::uninitialized_value_construct(this->mBack, this->mFront + count);
            } else if (count < this->ssize()) {
                shrinkSize(count);
            }

            this->mBack = this->mFront + count;
        }

        // change the size and capacity
        constexpr void resizeExact(SizeType count) noexcept {
            CTASSERTF(count >= 0, "Size must be non-negative: %zd", count);

            if (count > this->ssize()) {
                ensureGrowth(count);
                std::uninitialized_value_construct(this->mBack, this->mFront + count);
            } else if (count < this->ssize()) {
                shrinkCapacity(count);
            }

            this->mBack = this->mFront + count;
        }

        // appending

        constexpr T& emplace_back(auto&&... args) noexcept {
            ensureExtra(1);
            return *new (this->mBack++) T(std::forward<decltype(args)>(args)...);
        }

        constexpr T& emplace_back(T &&value) noexcept requires (std::is_move_constructible_v<T>) {
            ensureExtra(1);
            return (*this->mBack++ = std::move(value));
        }

        constexpr void push_back(const T &value) noexcept {
            ensureExtra(1);
            new (this->mBack++) T{value};
        }

        constexpr void push_back(T &&value) noexcept requires (std::is_move_constructible_v<T>) {
            ensureExtra(1);
            new (this->mBack++) T{std::move(value)};
        }

        constexpr void assign(const T *first, const T *last) noexcept {
            SizeType count = last - first;
            ensureExtra(count);
            std::move(first, last, this->mBack);
            this->mBack += count;
        }

        // swap

        friend constexpr void swap(VectorBase& lhs, VectorBase& rhs) noexcept {
            std::swap(lhs.mFront, rhs.mFront);
            std::swap(lhs.mBack, rhs.mBack);
            std::swap(lhs.mCapacity, rhs.mCapacity);
        }
    };

    template<typename T>
    class SmallVectorBase : public detail::Collection<T> {
        using Super = detail::Collection<T>;
        T *mCapacity;

    protected:
        constexpr SmallVectorBase(T *front, T *back, T *capacity) noexcept
            : Super(front, back)
            , mCapacity(capacity)
        { }

        constexpr void ensureGrowth(ssize_t size) noexcept {
            CTASSERTF(size <= capacity(), "Cannot add new element to SmallVector: %zd <= %zd", size, capacity());
        }

        constexpr void ensureExtra(ssize_t extra) noexcept {
            ensureGrowth(this->ssize() + extra);
        }
    public:
        constexpr ssize_t capacity() const noexcept { return mCapacity - this->mFront; }
    };

    template<typename T, size_t N>
    class SmallVectorBody final : public SmallVectorBase<T> {
        using Super = SmallVectorBase<T>;

        T mStorage[N];
    public:
        constexpr SmallVectorBody() noexcept
            : Super(mStorage, mStorage, mStorage + N)
        { }

        constexpr SmallVectorBody(std::initializer_list<T> init) noexcept
            : Super(mStorage, mStorage + init.size(), mStorage + N)
        {
            CTASSERTF(init.size() <= N, "Initializer list size must be less than or equal to N: %zu <= %zu", init.size(), N);
            std::uninitialized_copy(init.begin(), init.end(), this->mFront);
        }

        constexpr SmallVectorBody(const T *first, const T *last) noexcept
            : Super(mStorage, mStorage + (last - first), mStorage + N)
        {
            CTASSERTF(last - first <= N, "Range size must be less than or equal to N: %zu <= %zu", last - first, N);
            std::uninitialized_copy(first, last, this->mFront);
        }

        template<size_t M>
        constexpr SmallVectorBody(const T (&array)[M]) noexcept
            : Super(mStorage, mStorage + M, mStorage + N)
        {
            CTASSERTF(M <= N, "Array size must be less than or equal to N: %zu <= %zu", M, N);
            std::uninitialized_copy(array, array + M, this->mFront);
        }

        constexpr void emplace_back(auto&&... args) noexcept {
            this->ensureExtra(1);
            new (this->mBack++) T(std::forward<decltype(args)>(args)...);
        }

        constexpr void emplace_back(T &&value) noexcept requires (std::is_move_constructible_v<T>) {
            this->ensureExtra(1);
            new (this->mBack++) T(std::move(value));
        }

        constexpr void push_back(const T &value) noexcept {
            this->ensureExtra(1);
            new (this->mBack++) T{value};
        }

        constexpr void push_back(T &&value) noexcept requires (std::is_move_constructible_v<T>) {
            this->ensureExtra(1);
            new (this->mBack++) T{std::move(value)};
        }
    };
}
