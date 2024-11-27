#pragma once

#include "base/panic.h" // IWYU pragma: keep

#include "core/core.hpp"
#include "core/macros.hpp"
#include "core/traits.hpp"

#include <utility>

namespace sm {
    /// @brief A handle to a resource that is automatically destroyed when it goes out of scope.
    /// @tparam T The type of the handle.
    /// @tparam TDelete the deleter function that is called to destroy the handle.
    /// @tparam TEmpty The value that represents an empty handle.
    ///
    /// @a TEmpty is provided to allow for handles whos default value is not the same as the
    /// value that represents an empty handle. For example mmap returns MAP_FAILED on failure
    template<typename T, typename TDelete, T TEmpty = T()>
    class UniqueHandle {
        SM_NO_UNIQUE_ADDRESS TDelete mDelete{};

        T mHandle = TEmpty;

    public:
        constexpr UniqueHandle(T handle = TEmpty, TDelete del = TDelete{}) noexcept
            : mDelete(del)
            , mHandle(handle)
        { }

        constexpr UniqueHandle &operator=(T handle) noexcept {
            reset(handle);
            return *this;
        }

        constexpr ~UniqueHandle() noexcept {
            reset();
        }

        constexpr UniqueHandle(UniqueHandle &&other) noexcept {
            reset(other.release());
        }

        constexpr UniqueHandle &operator=(UniqueHandle &&other) noexcept {
            reset(other.release());
            return *this;
        }

        constexpr UniqueHandle(const UniqueHandle&) = delete;
        constexpr UniqueHandle &operator=(const UniqueHandle&) = delete;

        constexpr auto get(this auto& self) noexcept {
            CTASSERT(self.isValid());
            return self.mHandle;
        }

        constexpr auto& ref(this auto& self) noexcept {
            CTASSERT(self.isValid());
            return self.mHandle;
        }

        constexpr auto& operator*(this auto& self) noexcept {
            CTASSERT(self.isValid());
            return self.mHandle;
        }

        constexpr auto *address(this auto& self) noexcept { return &self.mHandle; }

        constexpr bool isValid() const noexcept { return mHandle != TEmpty; }
        constexpr explicit operator bool() const noexcept { return isValid(); }

        constexpr void reset(T handle = TEmpty) noexcept {
            if (mHandle != TEmpty)
                mDelete(mHandle);

            mHandle = handle;
        }

        constexpr T release() noexcept {
            T handle = mHandle;
            mHandle = TEmpty;
            return handle;
        }
    };

    template<typename T, void(*F)(T&), T TEmpty = {}>
    using FnUniqueHandle = UniqueHandle<T, decltype([](T& it) { F(it); }), TEmpty>;

    template<typename T>
    struct DefaultDelete {
        constexpr void operator()(T *data) const noexcept { delete data; }
    };

    template<typename T>
    struct DefaultDelete<T[]> {
        constexpr void operator()(T *data) const noexcept { delete[] data; }
    };

    template<typename T, typename TDelete>
    class UniquePtr;

    template<typename T, typename TDelete = DefaultDelete<T>>
    class UniquePtr : public UniqueHandle<T*, TDelete, nullptr> {
        using Super = UniqueHandle<T*, TDelete, nullptr>;
    public:
        using Super::Super;

        constexpr auto *operator->(this auto& self) noexcept { return self.get(); }
        constexpr auto** operator&(this auto& self) noexcept { return self.address(); }

        constexpr T& ref() noexcept { return *Super::get(); }
        constexpr const T& ref() const noexcept { return *Super::get(); }

        constexpr void reset(T *data = nullptr) noexcept {
            Super::reset(data);
        }

        constexpr bool operator==(const UniquePtr& other) const noexcept {
            return Super::get() == other.get();
        }

        constexpr bool operator==(std::nullptr_t) const noexcept {
            return Super::get() == nullptr;
        }
    };

    template<typename T, void(*F)(T*)>
    using FnUniquePtr = UniquePtr<T, decltype([](T* it) { F(it); })>;

    template<typename T, typename TDelete>
    class UniquePtr<T[], TDelete> : public UniquePtr<T, TDelete> {
        using Super = UniquePtr<T, TDelete>;

        DBG_MEMBER(size_t) mSize;

        constexpr void verifyIndex(SM_UNUSED size_t index) const noexcept {
            DBG_ASSERT(index < mSize, "index out of bounds (%zu < %zu)", index, mSize);
        }

    public:
        using Super::Super;

        constexpr UniquePtr(T *data, size_t size, TDelete del = TDelete{}) noexcept
            : Super(data, del)
            , mSize(size)
        { }

        constexpr UniquePtr() noexcept
            : UniquePtr(nullptr, 0)
        { }

        // TODO: arena aware allocation
        constexpr UniquePtr(size_t size)
            : UniquePtr(new T[size], size)
        { }

        constexpr void reset(size_t size) noexcept {
            reset(new T[size], size);
        }

        constexpr void reset(T *data, size_t size) noexcept {
            Super::reset(data);
            mSize = size;
        }

        constexpr auto &operator[](this auto& self, size_t index) noexcept {
            self.verifyIndex(index);
            return self.get()[index];
        }
    };

    template<typename T, typename TDelete = DefaultDelete<T>>
    constexpr UniquePtr<T, TDelete> makeUnique(T *data, TDelete del = TDelete{}) noexcept {
        return UniquePtr<T, TDelete>(data);
    }

    template<typename T, typename TDelete = DefaultDelete<T>>
    constexpr UniquePtr<T, TDelete> makeUnique(auto&&... args) {
        return UniquePtr<T, TDelete>(new T(std::forward<decltype(args)>(args)...));
    }

    template<typename T, typename TDelete = DefaultDelete<T>>
    constexpr UniquePtr<T, TDelete> makeUnique(T *data, size_t size, TDelete del = TDelete{}) noexcept {
        return UniquePtr<T, TDelete>(data, size);
    }

    template<IsArray T, typename TDelete = DefaultDelete<T>>
    constexpr UniquePtr<T, TDelete> makeUnique(size_t size) {
        using ElementType = std::remove_extent_t<T>;
        return UniquePtr<T, TDelete>(new ElementType[size], size);
    }

    DBG_STATIC_ASSERT(sizeof(sm::UniquePtr<int>) == sizeof(int*),
        "UniquePtr<T> should be the same size as T* in release"
        "a compiler that supports (and implements) [[no_unique_address]] or [[msvc::no_unique_address]] is required");
}

// NOLINTBEGIN

template<typename T>
struct std::tuple_size<sm::UniquePtr<T>> : std::tuple_size<T> { };

template<size_t I, typename T>
struct std::tuple_element<I, sm::UniquePtr<T>> : std::tuple_element<I, T> { };

namespace std {
    template<size_t I, typename T>
    constexpr decltype(auto) get(sm::UniquePtr<T>& ptr) noexcept {
        return std::get<I>(ptr.ref());
    }
}

// NOLINTEND
