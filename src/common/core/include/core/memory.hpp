#pragma once

#include <vector>

namespace sm {
    class DebugRegion;
    class DebugPtrBase;

    class DebugPtrBase {
        friend class DebugRegion;

        DebugRegion *m_owning = nullptr;

        constexpr void invalidate();

    protected:
        constexpr DebugPtrBase() = default;

        constexpr DebugPtrBase(DebugRegion *region)
            : m_owning(region)
        { }
    public:
    };

    template<typename T>
    class DebugHandle;

    template<typename T>
    class DebugPtr;

    class DebugRegion {
        // todo: use a flatmap
        std::vector<DebugPtrBase*> m_handles;

    public:
        constexpr DebugRegion() = default;
        ~DebugRegion() { invalidate(); }

        template<typename T>
        constexpr DebugPtr<T> create(T *ptr) {
            DebugHandle<T> *handle = new DebugHandle<T>(this, ptr);
            m_handles.push_back(handle);
            return handle;
        }

        void invalidate();
    };

    template<typename T>
    class DebugHandle : public DebugPtrBase {
        friend class DebugRegion;
        friend class DebugPtr<T>;

        T *m_object = nullptr;

        constexpr bool is_valid() const { return m_object != nullptr; }
    public:
        constexpr DebugHandle() = default;

        constexpr DebugHandle(DebugRegion *region, T *ptr)
            : DebugPtrBase(region)
            , m_object(ptr)
        { }
    };

    template<typename T>
    class DebugPtr {
    public:
        using Handle = DebugHandle<T>;

    private:
        Handle *m_handle = nullptr;

    public:
        constexpr DebugPtr() = default;
        constexpr DebugPtr(Handle *handle)
            : m_handle(handle)
        { }

        constexpr DebugPtr(const DebugPtr<T> &other) = default;
        constexpr DebugPtr<T> &operator=(const DebugPtr<T> &other) = default;

        constexpr DebugPtr(DebugPtr<T> &&other) = default;
        constexpr DebugPtr<T> &operator=(DebugPtr<T> &&other) = default;

        T *operator->() const { CTASSERTF(m_handle->is_valid(), "accessing object past lifetime"); return m_handle->m_object; }
        T &operator*() const { CTASSERTF(m_handle->is_valid(), "accessing object past lifetime"); return *m_handle->m_object; }
    };
}
