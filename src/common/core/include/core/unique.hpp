#pragma once

#include "base/panic.h" // IWYU pragma: keep

#include "core/core.hpp"
#include "core/macros.hpp"

namespace sm {
    /// @brief A handle to a resource that is automatically destroyed when it goes out of scope.
    /// @tparam T The type of the handle.
    /// @tparam TDelete the deleter function that is called to destroy the handle.
    /// @tparam TEmpty The value that represents an empty handle.
    ///
    /// @a TEmpty is provided to allow for handles whos default value is not the same as the
    /// value that represents an empty handle. For example mmap returns MAP_FAILED on failure
    template<typename T, typename TDelete, T TEmpty = T{}>
    class UniqueHandle {
        T m_handle = TEmpty;

        SM_NO_UNIQUE_ADDRESS TDelete m_delete{};

    public:
        constexpr UniqueHandle(T handle = TEmpty, TDelete del = TDelete{})
            : m_handle(handle)
            , m_delete(del)
        { }

        constexpr UniqueHandle &operator=(T handle) {
            reset(handle);
            return *this;
        }

        constexpr ~UniqueHandle() {
            reset();
        }

        constexpr UniqueHandle(UniqueHandle &&other) {
            reset(other.release());
        }

        constexpr UniqueHandle &operator=(UniqueHandle &&other) {
            reset(other.release());
            return *this;
        }

        constexpr T& get() { CTASSERT(is_valid()); return m_handle; }
        constexpr const T& get() const { CTASSERT(is_valid()); return m_handle; }

        constexpr T& operator*() { return get(); }
        constexpr const T& operator*() const { return get(); }

        constexpr T *address() { return &m_handle; }
        constexpr T *const address() const { return &m_handle; }

        constexpr bool is_valid() const { return m_handle != TEmpty; }
        constexpr explicit operator bool() const { return m_handle != TEmpty; }

        constexpr void reset(T handle = TEmpty) {
            if (m_handle != TEmpty) {
                m_delete(m_handle);
            }
            m_handle = handle;
        }

        constexpr T release() {
            T handle = m_handle;
            m_handle = TEmpty;
            return handle;
        }
    };

    template<typename T>
    struct DefaultDelete {
        constexpr void operator()(T *data) const { delete data; }
    };

    template<typename T>
    struct DefaultDelete<T[]> {
        constexpr void operator()(T *data) const { delete[] data; }
    };

    template<typename T, typename TDelete>
    class UniquePtr;

    template<typename T, typename TDelete = DefaultDelete<T>>
    class UniquePtr : public UniqueHandle<T*, TDelete, nullptr> {
        using Super = UniqueHandle<T*, TDelete, nullptr>;
    public:
        using Super::Super;

        constexpr T *operator->() { return Super::get(); }
        constexpr const T *operator->() const { return Super::get(); }

        constexpr T** operator&() { return Super::address(); }
        constexpr T* const* operator&() const { return Super::address(); }

        constexpr void reset(T *data = nullptr) {
            Super::reset(data);
        }
    };

    template<typename T, void(*F)(T*)>
    using FnUniquePtr = UniquePtr<T, decltype([](T* it) { F(it); })>;

    template<typename T, typename TDelete>
    class UniquePtr<T[], TDelete> : public UniquePtr<T, TDelete> {
        using Super = UniquePtr<T, TDelete>;
    public:
        using Super::Super;

        constexpr UniquePtr(T *data, size_t size)
            : Super(data)
            , m_size(size)
        { }

        constexpr UniquePtr()
            : UniquePtr(nullptr, 0)
        { }

        // TODO: arena aware allocation
        constexpr UniquePtr(size_t size)
            : UniquePtr(new T[size], size)
        { }

        constexpr void reset(size_t size) {
            Super::reset(new T[size]);
            SM_DBG_REF(m_size) = size;
        }

        constexpr T &operator[](size_t index) {
            verify_index(index);
            return Super::get()[index];
        }

        constexpr const T& operator[](size_t index) const {
            verify_index(index);
            return Super::get()[index];
        }

    private:
        constexpr void verify_index(SM_UNUSED size_t index) const {
            SM_DBG_ASSERT(index < m_size, "index out of bounds (%zu < %zu)", index, m_size);
        }

        DBG_MEMBER(size_t) m_size;
    };

    DBG_STATIC_ASSERT(sizeof(sm::UniquePtr<int>) == sizeof(int*),
        "UniquePtr<T> should be the same size as T* in release"
        "a compiler that supports [[no_unique_address]] or [[msvc::no_unique_address]] is required");
}
