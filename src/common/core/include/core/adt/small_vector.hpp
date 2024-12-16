#pragma once

#include "base/macros.hpp"

#include "core/adt/range.hpp"
#include "core/core.hpp"

#include "base/panic.h"

#include <memory>
#include <algorithm>

namespace sm {
    template<typename T>
    class SmallVectorBase : public core::detail::Collection<T> {
        using Super = core::detail::Collection<T>;
        T *mCapacity;

        static_assert(std::is_nothrow_move_constructible_v<T>);
        static_assert(std::is_nothrow_destructible_v<T>);

    protected:
        constexpr SmallVectorBase(T *front, T *back, T *capacity) noexcept
            : Super(front, back)
            , mCapacity(capacity)
        { }

        [[nodiscard]] constexpr bool isValidSize(ssize_t size) const noexcept {
            return size >= 0 && size <= capacity();
        }

        constexpr void verifySize(ssize_t size) const noexcept {
            CTASSERTF(isValidSize(size), "Size must be in range [0, %zd]: %zd", capacity(), size);
        }

        constexpr void ensureExtra(ssize_t extra) const noexcept {
            verifySize(this->ssize() + extra);
        }

        constexpr void moveTail(size_t count) noexcept {
            verifySize(count);
            this->mBack = this->mFront + count;
        }

        constexpr void constructByCopy(const T *first, const T *last) noexcept {
            std::uninitialized_copy(first, last, this->mFront);
        }

        constexpr void initByCopy(const T *first, const T *last) noexcept {
            resize(last - first);
            std::copy(first, last, this->mFront);
        }

        constexpr void constructByMove(T *first, T *last) noexcept {
            std::uninitialized_move(first, last, this->mFront);
        }

        constexpr void initByMove(T *first, T *last) noexcept {
            resize(last - first);
            std::move(first, last, this->mFront);
        }

    public:
        [[nodiscard]] constexpr ssize_t capacity() const noexcept { return mCapacity - this->mFront; }

        constexpr ~SmallVectorBase() noexcept {
            std::destroy(this->mFront, this->mBack);
            this->mBack = this->mFront;
        }

        constexpr T& emplace_back(auto&&... args) noexcept {
            ensureExtra(1);
            return *std::construct_at(this->mBack++, std::forward<decltype(args)>(args)...);
        }

        constexpr T& emplace_back(T &&value) noexcept requires (std::is_move_constructible_v<T>) {
            ensureExtra(1);
            return *std::construct_at(this->mBack++, std::move(value));
        }

        constexpr void push_back(const T &value) noexcept {
            ensureExtra(1);
            new (this->mBack++) T{value};
        }

        constexpr void push_back(T &&value) noexcept requires (std::is_move_constructible_v<T>) {
            ensureExtra(1);
            new (this->mBack++) T{std::move(value)};
        }

        constexpr void pop_back() noexcept {
            CTASSERTF(this->ssize() > 0, "Cannot pop back from empty vector");
            std::destroy_at(--this->mBack);
        }

        constexpr bool tryPopBack() noexcept {
            if (this->ssize() > 0) {
                pop_back();
                return true;
            }

            return false;
        }

        constexpr void clear() noexcept {
            std::destroy(this->mFront, this->mBack);
            this->mBack = this->mFront;
        }

        constexpr bool tryPushBack(const T &value) noexcept {
            if (this->ssize() < capacity()) {
                push_back(value);
                return true;
            }

            return false;
        }

        constexpr bool tryPushBack(T &&value) noexcept requires (std::is_move_constructible_v<T>) {
            if (this->ssize() < capacity()) {
                push_back(std::move(value));
                return true;
            }

            return false;
        }

        constexpr bool tryEmplaceBack(auto&&... args) noexcept {
            if (this->ssize() < capacity()) {
                emplace_back(std::forward<decltype(args)>(args)...);
                return true;
            }

            return false;
        }

        constexpr bool tryEmplaceBack(T &&value) noexcept requires (std::is_move_constructible_v<T>) {
            if (this->ssize() < capacity()) {
                emplace_back(std::move(value));
                return true;
            }

            return false;
        }

        constexpr void resize(ssize_t size) noexcept {
            verifySize(size);

            if (size > this->ssize()) {
                std::uninitialized_value_construct(this->mBack, this->mFront + size);
            } else if (size < this->ssize()) {
                std::destroy(this->mFront + size, this->mBack);
            }

            this->mBack = this->mFront + size;
        }

        constexpr bool tryResize(ssize_t size) noexcept {
            if (isValidSize(size)) {
                resize(size);
                return true;
            }

            return false;
        }
    };

    /// @brief A small vector with a fixed capacity
    ///
    /// @tparam T the element type
    /// @tparam N the maximum number of elements
    template<typename T, ssize_t N> requires (N >= 0)
    class SmallVector final : public SmallVectorBase<T> {
        using Super = SmallVectorBase<T>;

        // use a union to avoid default construction of the storage
        union Storage {
            T data[N];

            constexpr Storage() noexcept { }
            constexpr ~Storage() noexcept { }
        };

        Storage mStorage;

    public:
        constexpr SmallVector(ssize_t size, noinit) noexcept
            : Super(mStorage.data, mStorage.data + size, mStorage.data + N)
        {
            Super::verifySize(size);
        }

        constexpr SmallVector() noexcept
            : SmallVector(0, noinit{})
        { }

        constexpr SmallVector(const T *first, const T *last) noexcept
            : SmallVector(last - first, noinit{})
        {
            Super::constructByCopy(first, last);
        }

        constexpr SmallVector(std::initializer_list<T> init) noexcept
            : SmallVector(init.begin(), init.end())
        { }

        template<size_t M> requires (M <= N)
        constexpr SmallVector(const T (&array)[M]) noexcept
            : SmallVector(array, array + M)
        { }

        /// Move constructor

        constexpr SmallVector(Super&& other) noexcept
            : SmallVector(other.ssize(), noinit{})
        {
            Super::constructByMove(other.begin(), other.end());
            other.clear();
        }

        constexpr SmallVector(SmallVector&& other) noexcept
            : SmallVector((Super&&)other)
        { }

        /// Move assignment

        constexpr SmallVector& operator=(Super&& other) noexcept {
            if (this == &other)
                return *this;

            Super::initByMove(other.begin(), other.end());
            other.clear();

            return *this;
        }

        // NOLINTNEXTLINE(cert-oop54-cpp) - operator=(Super&&) handles self assignment
        constexpr SmallVector& operator=(SmallVector&& other) noexcept {
            return *this = (Super)other;
        }

        /// Copy constructor

        explicit constexpr SmallVector(const Super &other) noexcept
            : SmallVector(other.begin(), other.end())
        { }

        explicit constexpr SmallVector(const SmallVector &other) noexcept
            : SmallVector((const Super&)other)
        { }

        /// Copy assignment

        constexpr SmallVector& operator=(const Super& other) noexcept {
            if (this == &other)
                return *this;

            Super::initByCopy(other.begin(), other.end());

            return *this;
        }

        constexpr SmallVector& operator=(const SmallVector& other) noexcept = SM_DELETE_REASON("copy using clone()");

        constexpr SmallVector clone() const noexcept {
            return SmallVector(*this);
        }
    };
}
