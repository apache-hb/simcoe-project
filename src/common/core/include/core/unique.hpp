#pragma once

#include <simcoe_config.h>

#include "base/panic.h" // IWYU pragma: keep

#include "core/macros.hpp"

namespace sm {
    // TODO: arena support
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
        SM_NOCOPY(UniqueHandle)

        constexpr UniqueHandle(T handle = TEmpty)
            : m_handle(handle)
        { }

        constexpr UniqueHandle &operator=(T handle) {
            reset(handle);
            return *this;
        }

        constexpr ~UniqueHandle() {
            destroy();
        }

        constexpr UniqueHandle(UniqueHandle &&other) {
            reset(other.m_handle);
            other.m_handle = TEmpty;
        }

        constexpr UniqueHandle &operator=(UniqueHandle &&other) {
            reset(other.m_handle);
            other.m_handle = TEmpty;
            return *this;
        }

        constexpr void destroy() {
            if (m_handle != TEmpty) {
                m_delete(m_handle);
                m_handle = TEmpty;
            }
        }

        constexpr T& get() { CTASSERT(is_valid()); return m_handle; }
        constexpr const T& get() const { CTASSERT(is_valid()); return m_handle; }
        constexpr explicit operator bool() const { return m_handle != TEmpty; }

        constexpr bool is_valid() const { return m_handle != TEmpty; }

        constexpr void reset(T handle = TEmpty) {
            if (m_handle != TEmpty) {
                m_delete(m_handle);
            }
            m_handle = handle;
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
    public:
        using Super = UniqueHandle<T*, TDelete, nullptr>;
        using Super::Super;

        constexpr T *operator->() { return Super::get(); }
        constexpr const T *operator->() const { return Super::get(); }

        constexpr void reset(T *data) {
            Super::destroy();
            Super::reset(data);
        }
    };

    template<typename T, typename TDelete>
    class UniquePtr<T[], TDelete> : public UniquePtr<T, TDelete> {
    public:
        using Super = UniquePtr<T, TDelete>;
        using Super::Super;

        constexpr UniquePtr(T *data, SM_UNUSED size_t size)
            : Super(data)
#if SMC_DEBUG
            , m_size(size)
#endif
        { }

        constexpr UniquePtr()
            : UniquePtr(nullptr, 0)
        { }

        constexpr UniquePtr(size_t size)
            : UniquePtr(new T[size], size)
        { }

        constexpr void reset(SM_UNUSED size_t size) {
            Super::destroy();
            Super::reset(new T[size]);
#if SMC_DEBUG
            m_size = size;
#endif
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
#if SMC_DEBUG
        constexpr void verify_index(size_t index) const {
            CTASSERTF(index < m_size, "index out of bounds (%zu < %zu)", index, m_size);
        }
        size_t m_size;
#else
        constexpr void verify_index(size_t) const { }
#endif
    };
}
