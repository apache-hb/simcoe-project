#pragma once

#include <simcoe_config.h>

#include <vector>

#include "core/bitmap.hpp"
#include "core/text.hpp"
#include "core/macros.hpp"

#include "core/unique.hpp"
#include "system/system.hpp" // IWYU pragma: export

#include "math/math.hpp"

#include "rhi.reflect.h"

///
/// rhi layout
/// Context
/// * dxgi factory, adapter enumeration.
/// Device
/// * d3d12 device, swapchain, queues, lists, fences, heaps, etc.
///

namespace sm::rhi {
    class Context;
    class Factory;
    class DescriptorHeap;

    using RenderSink = logs::Sink<logs::Category::eRender>;

    template<typename T>
    static constexpr DXGI_FORMAT kDataType = DXGI_FORMAT_UNKNOWN;

    template<> static constexpr DXGI_FORMAT kDataType<math::float2> = DXGI_FORMAT_R32G32_FLOAT;
    template<> static constexpr DXGI_FORMAT kDataType<math::float3> = DXGI_FORMAT_R32G32B32_FLOAT;
    template<> static constexpr DXGI_FORMAT kDataType<math::float4> = DXGI_FORMAT_R32G32B32A32_FLOAT;

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

        void rename(std::string_view name) requires (D3DObject<T>) {
            CTASSERT(is_valid());
            m_object->SetName(sm::widen(name).c_str());
        }
    };

    using CommandList = Object<ID3D12GraphicsCommandList2>;
    using Device = Object<ID3D12Device1>;

    class PipelineState {
        friend class Context;

        Object<ID3D12PipelineState> m_state;
        Object<ID3D12RootSignature> m_signature;
    };

    class Resource {
        Object<ID3D12Resource> m_resource;
    };

    class Adapter : public Object<IDXGIAdapter1> {
        DXGI_ADAPTER_DESC1 m_desc;
        std::string m_name;

    public:
        SM_NOCOPY(Adapter)

        Adapter(IDXGIAdapter1 *adapter) : Object(adapter) {
            if (is_valid()) {
                (*this)->GetDesc1(&m_desc);
                m_name = sm::narrow(m_desc.Description);
            }
        }

        Adapter(Adapter&& other) noexcept
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

    using DescriptorIndex = sm::BitMap::Index;

    class DescriptorHeap {
        friend class Context;

        // TODO: maybe use a slotmap for more resource safety
        sm::BitMap m_slots{};
        Object<ID3D12DescriptorHeap> m_heap{};
        UINT m_descriptor_size{};

        void init(ID3D12Device1 *device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT length, bool shader_visible);

    public:
        DescriptorHeap() = default;
        SM_NOCOPY(DescriptorHeap)
        SM_NOMOVE(DescriptorHeap)

        D3D12_GPU_DESCRIPTOR_HANDLE get_gpu_handle(DescriptorIndex index) const {
            CTASSERT(m_heap.is_valid());

            D3D12_GPU_DESCRIPTOR_HANDLE handle = m_heap->GetGPUDescriptorHandleForHeapStart();
            handle.ptr += index.as_integral() * m_descriptor_size;
            return handle;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE get_cpu_handle(DescriptorIndex index) const {
            CTASSERT(m_heap.is_valid());

            D3D12_CPU_DESCRIPTOR_HANDLE handle = m_heap->GetCPUDescriptorHandleForHeapStart();
            handle.ptr += index.as_integral() * m_descriptor_size;
            return handle;
        }

        D3D12_GPU_DESCRIPTOR_HANDLE get_gpu_front() const {
            CTASSERT(m_heap.is_valid());

            return m_heap->GetGPUDescriptorHandleForHeapStart();
        }

        D3D12_CPU_DESCRIPTOR_HANDLE get_cpu_front() const {
            CTASSERT(m_heap.is_valid());

            return m_heap->GetCPUDescriptorHandleForHeapStart();
        }

        DescriptorIndex alloc_index();
        UINT get_descriptor_size() const { return m_descriptor_size; }
        ID3D12DescriptorHeap *get_heap() const { return m_heap.get(); }
    };

    class CommandQueue {
        friend class Context;

        Object<ID3D12CommandQueue> m_queue;

        Object<ID3D12Fence> m_fence;
        HANDLE m_event = nullptr;

        void init(ID3D12Device1 *device, D3D12_COMMAND_LIST_TYPE type, const char *name = nullptr);

        ID3D12CommandQueue *get() const { return m_queue.get(); }
        void execute_commands(ID3D12CommandList *const *commands, UINT count);

        void signal(UINT64 value);
        void wait(UINT64 value);

    public:
        ~CommandQueue() { if (m_event) CloseHandle(m_event); }
    };

    struct Vertex {
        math::float3 position;
        math::float4 colour;
    };

    class Context {
        RenderConfig m_config;
        RenderSink m_log;

        Factory& m_factory;
        Adapter& m_adapter;

        Device m_device;
        Object<ID3D12Debug> m_debug;
        Object<ID3D12InfoQueue1> m_info_queue;
        DWORD m_cookie = 0;

        CommandQueue m_direct_queue;
        CommandQueue m_copy_queue;
        UINT64 m_copy_value = 0;

        Object<ID3D12GraphicsCommandList2> m_commands;
        Object<IDXGISwapChain4> m_swapchain;
        math::uint2 m_swapchain_size;

        Object<ID3D12CommandAllocator> m_copy_allocator;
        Object<ID3D12GraphicsCommandList2> m_copy_commands;

        DescriptorHeap m_rtv_heap{};
        DescriptorHeap m_srv_heap{};

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
        void setup_render_targets();
        void setup_direct_allocators();
        void setup_copy_queue();

        // setup assets that only depend on the device
        void setup_device_assets();

        // setup assets that depend on the swapchain resolution
        void setup_display_assets();

        void record_commands();
        void finish_commands();
        void submit_commands();

        void await_copy_queue();
        void await_direct_queue();

        sys::Window& get_window() const { return m_config.window; }

    public:
        SM_NOCOPY(Context)

        Context(const RenderConfig& config, Factory& factory);
        ~Context();

        const RenderConfig& get_config() const { return m_config; }
        DescriptorHeap& get_rtv_heap() { return m_rtv_heap; }
        DescriptorHeap& get_srv_heap() { return m_srv_heap; }
        ID3D12Device1 *get_device() const { return m_device.get(); }
        CommandList& get_commands() { return m_commands; }

        void resize(math::uint2 size);

        float get_aspect_ratio() const;

        void begin_frame();
        void end_frame();

        void present();
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
