#pragma once

#include <vector>

#include "base/panic.h"
#include "core/macros.hpp"

#include "core/text.hpp"
#include "rhi.reflect.h"

///
/// rhi layout
/// Context
/// * dxgi factory, adapter enumeration, etc.
/// Device
/// * d3d12 device, swapchain, direct command queue.
///

namespace sm::rhi {
    template<typename T>
    concept ComObject = std::is_base_of_v<IUnknown, T>;

    template<ComObject T>
    class Object {
        T *m_object;

    public:
        SM_NOCOPY(Object)

        constexpr Object(T *object = nullptr) : m_object(object) {}
        ~Object() { try_release(); }

        constexpr Object(Object&& other) noexcept
            : m_object(other.m_object)
        {
            other.m_object = nullptr;
        }

        constexpr Object& operator=(Object&& other) noexcept {
            try_release();
            m_object = other.m_object;
            other.m_object = nullptr;
            return *this;
        }

        void release() { CTASSERT(is_valid()); m_object->Release(); m_object = nullptr; }
        bool try_release() { if (is_valid()) { release(); return true; } return false; }

        constexpr T *operator->() const { CTASSERT(is_valid()); return m_object; }
        constexpr T **operator&() { return &m_object; }
        constexpr T *get() const { CTASSERT(is_valid()); return m_object; }
        constexpr T **get_address() { return &m_object; }

        constexpr bool is_valid() const { return m_object != nullptr; }
        constexpr operator bool() const { return is_valid(); }
    };

    class Device : public Object<ID3D12Device1> {
    public:
        SM_NOCOPY(Device)

        constexpr Device(ID3D12Device1 *device = nullptr) : Object(device) {}
    };

    class Adapter : public Object<IDXGIAdapter1> {
        DXGI_ADAPTER_DESC1 m_desc;
        std::string m_name;

    public:
        SM_NOCOPY(Adapter)

        constexpr Adapter(IDXGIAdapter1 *adapter) : Object(adapter) {
            if (is_valid()) {
                (*this)->GetDesc1(&m_desc);
                m_name = sm::narrow(m_desc.Description);
            }
        }

        constexpr Adapter(Adapter&& other) noexcept
            : Object(other.get())
            , m_desc(other.m_desc)
            , m_name(std::move(other.m_name))
        {
            other.release();
        }

        constexpr const DXGI_ADAPTER_DESC1& get_desc() const { return m_desc; }
        constexpr std::string_view get_adapter_name() const { return m_name; }
    };

    class Context {
        RenderConfig m_config;
        logs::Sink<logs::Category::eRender> m_log;

        Object<IDXGIFactory4> m_factory;
        Object<IDXGIDebug1> m_factory_debug;

        std::vector<Adapter> m_adapters;
        Device m_device;

        void enum_adapters();
        void enum_warp_adapter();

        void init();

    public:
        SM_NOCOPY(Context)
        SM_NOMOVE(Context)

        Context(const RenderConfig& config);
        ~Context();

        const RenderConfig& get_config() const { return m_config; }
    };
}
