#pragma once

#include <vector>

#include "core/text.hpp"
#include "core/macros.hpp"

#include "core/unique.hpp"
#include "system/system.hpp" // IWYU pragma: export

#include "math/math.hpp"

#include "rhi.reflect.h"

///
/// rhi layout
/// Context
/// * dxgi factory, adapter enumeration, etc.
/// Device
/// * d3d12 device, swapchain, direct command queue.
///

namespace sm::rhi {
    using RenderSink = logs::Sink<logs::Category::eRender>;

    template<typename T>
    concept ComObject = std::is_base_of_v<IUnknown, T>;

    template<typename T>
    concept D3DObject = std::is_base_of_v<ID3D12Object, T>;

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

        template<ComObject O>
        HRESULT query(O **out) const { return m_object->QueryInterface(IID_PPV_ARGS(out)); }

        void release() { CTASSERT(is_valid()); m_object->Release(); m_object = nullptr; }
        bool try_release() { if (is_valid()) { release(); return true; } return false; }

        constexpr T *operator->() const { CTASSERT(is_valid()); return m_object; }
        constexpr T **operator&() { return &m_object; }
        constexpr T *get() const { CTASSERT(is_valid()); return m_object; }
        constexpr T **get_address() { return &m_object; }

        constexpr bool is_valid() const { return m_object != nullptr; }
        constexpr operator bool() const { return is_valid(); }
    };

    class PipelineState {
        friend class Context;

        Object<ID3D12PipelineState> m_state;
        Object<ID3D12RootSignature> m_signature;
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

    struct FrameData {
        Object<ID3D12Resource> backbuffer;
        Object<ID3D12CommandAllocator> allocator;
        UINT64 fence_value = 1;
    };

    struct Vertex {
        math::float3 position;
        math::float4 colour;
    };

    class Factory;

    class Context {
        RenderConfig m_config;
        RenderSink m_log;

        Factory& m_factory;
        Adapter& m_adapter;

        Object<ID3D12Device1> m_device;
        Object<ID3D12Debug> m_debug;
        Object<ID3D12InfoQueue1> m_info_queue;
        DWORD cookie = 0;

        Object<ID3D12CommandQueue> m_queue;
        Object<ID3D12GraphicsCommandList2> m_commands;
        Object<IDXGISwapChain4> m_swapchain;

        Object<ID3D12Fence> m_fence;
        HANDLE m_fevent = nullptr;

        Object<ID3D12DescriptorHeap> m_rtv_heap;
        UINT m_rtv_descriptor_size = 0;

        UINT m_frame_index = 0;
        sm::UniquePtr<FrameData[]> m_frames;

        // viewport/scissor
        D3D12_VIEWPORT m_viewport;
        D3D12_RECT m_scissor;

        // pipeline
        Object<ID3D12PipelineState> m_state;
        Object<ID3D12RootSignature> m_signature;

        // gpu resources
        Object<ID3D12Resource> m_vbo;
        D3D12_VERTEX_BUFFER_VIEW m_vbo_view;

        static void info_callback(D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID id, LPCSTR desc, void *user);

        void init();
        void setup_device();
        void setup_pipeline();
        void setup_assets();

        void setup_commands();
        void submit_commands();

        void wait_for_fence();

        sys::Window& get_window() const { return m_config.window; }

    public:
        SM_NOCOPY(Context)

        Context(const RenderConfig& config, Factory& factory);
        ~Context();

        const RenderConfig& get_config() const { return m_config; }

        float get_aspect_ratio() const;

        void begin_frame();
        void end_frame();
    };

    class Factory {
        friend class Context;

        RenderConfig m_config;
        RenderSink m_log;

        Object<IDXGIFactory4> m_factory;
        Object<IDXGIDebug1> m_factory_debug;
        std::vector<Adapter> m_adapters;

        void enum_adapters();
        void enum_warp_adapter();
        void init();

        Adapter& get_selected_adapter();

    public:
        SM_NOCOPY(Factory)

        Factory(const RenderConfig& config);
        ~Factory();

        const RenderConfig& get_config() const { return m_config; }

        Context new_context();
    };
}
