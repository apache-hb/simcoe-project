#pragma once

#include <sm_rhi_api.hpp>
#include <vector>

#include "base/panic.h"

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
    class SM_RHI_API Object {
        T *m_object;

    public:
        SM_NOCOPY(Object)
        SM_NOMOVE(Object)

        constexpr Object(T *object = nullptr) : m_object(object) {}
        ~Object() { if (is_valid()) m_object->Release(); }

        constexpr T *operator->() const { CTASSERT(is_valid()); return m_object; }
        constexpr T *get() const { CTASSERT(is_valid()); return m_object; }

        constexpr bool is_valid() const { return m_object != nullptr; }
        constexpr operator bool() const { return is_valid(); }
    };

    class SM_RHI_API Device {

    };

    class SM_RHI_API Context {
        RenderConfig m_config;

        Object<IDXGIFactory4> m_factory;
        std::vector<Object<IDXGIAdapter1>> m_adapters;
        Object<ID3D12Device1> m_device;

    public:
        SM_NOCOPY(Context)
        SM_NOMOVE(Context)

        Context(const RenderConfig& config);
        ~Context();

        const RenderConfig& get_config() const { return m_config; }
    };
}
