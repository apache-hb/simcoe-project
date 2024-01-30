#pragma once

#include <simcoe_config.h>
#include <span>

#include "core/bitmap.hpp"
#include "core/text.hpp"
#include "core/macros.hpp"
#include "core/vector.hpp"

#include "core/unique.hpp"

#include "math/math.hpp"

#include "rhi.reflect.h"

///
/// rhi layout
/// Context
/// * dxgi factory, adapter enumeration.
/// Device
/// * d3d12 device, swapchain, queues, lists, fences, heaps, etc.
///

#define CBUFFER_ALIGN alignas(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)

#define SM_ASSERT_HR(expr)                                 \
    do {                                                   \
        if (auto result = (expr); FAILED(result)) {        \
            sm::rhi::assert_hresult(CT_SOURCE_HERE, #expr, result); \
        }                                                  \
    } while (0)

namespace sm::rhi {
    class Device;
    class Factory;

    CT_NORETURN assert_hresult(source_info_t source, const char *expr, HRESULT hr);
    char *hresult_string(HRESULT hr);

    DXGI_FORMAT get_data_format(bundle::DataFormat format);

    using RhiSink = logs::Sink<logs::Category::eRHI>;

    class Adapter : public AdapterObject {
        DXGI_ADAPTER_DESC1 m_desc;
        sm::String m_name;

    public:
        SM_NOCOPY(Adapter)

        Adapter(IDXGIAdapter1 *adapter) : AdapterObject(adapter) {
            if (is_valid()) {
                (*this)->GetDesc1(&m_desc);
                m_name = sm::narrow(m_desc.Description);
            }
        }

        Adapter(Adapter&& other)
            : Object(other.acquire())
            , m_desc(other.m_desc)
            , m_name(std::move(other.m_name))
        { }

        constexpr const DXGI_ADAPTER_DESC1& get_desc() const { return m_desc; }
        constexpr std::string_view get_adapter_name() const { return m_name; }
    };

    using DescriptorIndex = sm::BitMap::Index;

    class CpuDescriptorHandle : public D3D12_CPU_DESCRIPTOR_HANDLE {
        friend class DescriptorHeap;

        CpuDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle, SIZE_T offset, UINT stride)
            : D3D12_CPU_DESCRIPTOR_HANDLE(handle.ptr + offset * stride)
        { }
    };

    class GpuDescriptorHandle : public D3D12_GPU_DESCRIPTOR_HANDLE {
        friend class DescriptorHeap;

        GpuDescriptorHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle, SIZE_T offset, UINT stride)
            : D3D12_GPU_DESCRIPTOR_HANDLE(handle.ptr + offset * stride)
        { }
    };

    class DescriptorHeap {
        DescriptorHeapObject m_heap;
        UINT m_descriptor_size = 0;

    public:
        void init(Device& device, const DescriptorHeapConfig& config);

        constexpr bool is_valid() const { return m_heap.is_valid(); }
        ID3D12DescriptorHeap *get() const { return m_heap.get(); }

        CpuDescriptorHandle cpu_front() const {
            return CpuDescriptorHandle(m_heap->GetCPUDescriptorHandleForHeapStart(), 0, m_descriptor_size);
        }

        GpuDescriptorHandle gpu_front() const {
            return GpuDescriptorHandle(m_heap->GetGPUDescriptorHandleForHeapStart(), 0, m_descriptor_size);
        }

        CpuDescriptorHandle cpu_handle(DescriptorIndex index) const {
            return CpuDescriptorHandle(m_heap->GetCPUDescriptorHandleForHeapStart(), index, m_descriptor_size);
        }

        GpuDescriptorHandle gpu_handle(DescriptorIndex index) const {
            return GpuDescriptorHandle(m_heap->GetGPUDescriptorHandleForHeapStart(), index, m_descriptor_size);
        }
    };

    class Resource {
        friend class Device;

        ResourceObject m_resource;

    public:
        template<typename T>
        void write(std::span<const T> data) {
            void *ptr = nullptr;
            SM_ASSERT_HR(m_resource->Map(0, nullptr, &ptr));
            std::memcpy(ptr, data.data(), data.size_bytes());
            m_resource->Unmap(0, nullptr);
        }
    };

    // TODO: remove this from rhi
    class DescriptorArena {
        friend class Device;

        // TODO: maybe use a slotmap for more resource safety
        sm::BitMap m_slots{};
        DescriptorHeap m_heap{};

        void init(Device& device, const DescriptorHeapConfig& config);

    public:
        DescriptorArena() = default;
        SM_NOCOPY(DescriptorArena)
        SM_NOMOVE(DescriptorArena)

        D3D12_GPU_DESCRIPTOR_HANDLE get_gpu_handle(DescriptorIndex index) const {
            CTASSERT(index != DescriptorIndex::eInvalid);

            return m_heap.gpu_handle(index);
        }

        D3D12_CPU_DESCRIPTOR_HANDLE get_cpu_handle(DescriptorIndex index) const {
            CTASSERT(index != DescriptorIndex::eInvalid);

            return m_heap.cpu_handle(index);
        }

        D3D12_GPU_DESCRIPTOR_HANDLE get_gpu_front() const {
            CTASSERT(m_heap.is_valid());

            return m_heap.gpu_front();
        }

        D3D12_CPU_DESCRIPTOR_HANDLE get_cpu_front() const {
            CTASSERT(m_heap.is_valid());

            return m_heap.cpu_front();
        }

        void release_index(DescriptorIndex index);
        DescriptorIndex alloc_index();
        ID3D12DescriptorHeap *get_heap() const { return m_heap.get(); }
    };

    class PipelineState {
        friend class Device;

        RootSignatureObject m_signature;
        PipelineStateObject m_pipeline;

    public:
        SM_NOCOPY(PipelineState)
        PipelineState() = default;

        void init(Device& device, const GraphicsPipelineConfig& config);

        ID3D12RootSignature *get_root_signature() const { return m_signature.get(); }
        ID3D12PipelineState *get_pipeline() const { return m_pipeline.get(); }
    };

    class Allocator : public AllocatorObject {
        friend class Device;
        friend class CommandList;

        void init(Device& device, CommandListType type);
    };

    class CommandList : public CommandListObject {
        friend class Device;

    public:
        SM_NOCOPY(CommandList)
        CommandList() = default;

        void init(Device& device, Allocator& allocator, CommandListType type);
        void init(Device& device, Allocator& allocator, CommandListType type, PipelineState& pipeline);

        void close() { SM_ASSERT_HR((*this)->Close()); }

        void reset(Allocator& allocator, PipelineState& pipeline) {
            SM_ASSERT_HR((*this)->Reset(allocator.get(), pipeline.get_pipeline()));
        }

        void reset(Allocator& allocator) {
            SM_ASSERT_HR((*this)->Reset(allocator.get(), nullptr));
        }

        void set_root_signature(const PipelineState& pipeline) {
            (*this)->SetGraphicsRootSignature(pipeline.get_root_signature());
        }

        void set_pipeline_state(const PipelineState& pipeline) {
            (*this)->SetPipelineState(pipeline.get_pipeline());
        }
    };

    class CommandQueue {
        friend class Device;

        CommandQueueObject m_queue;

        FenceObject m_fence;
        HANDLE m_event = nullptr;

        void init(Device& device, CommandListType type, const char *name = nullptr);

        ID3D12CommandQueue *get() const { return m_queue.get(); }
        void execute_commands(ID3D12CommandList *const *commands, UINT count);

        void signal(UINT64 value);
        void wait(UINT64 value);

    public:
        SM_NOCOPY(CommandQueue)
        CommandQueue() = default;

        ~CommandQueue() { if (m_event) CloseHandle(m_event); }
    };

    struct FrameData {
        ResourceObject backbuffer;
        DescriptorIndex rtv_index = DescriptorIndex::eInvalid;
        Allocator allocator;
        UINT64 fence_value = 1;
    };

    class Device {
        RenderConfig m_config;
        RhiSink m_log;

        Factory& m_factory;
        Adapter& m_adapter;

        RootSignatureVersion m_version = RootSignatureVersion::eVersion_1_0;
        DeviceObject m_device;
        Object<ID3D12Debug> m_debug;
        Object<ID3D12InfoQueue1> m_info_queue;
        DWORD m_cookie = 0;

        CommandQueue m_direct_queue;
        CommandQueue m_copy_queue;
        UINT64 m_copy_value = 1;

        CommandList m_direct_list;

        Allocator m_copy_allocator;
        CommandList m_copy_list;

        Object<IDXGISwapChain4> m_swapchain;
        math::uint2 m_swapchain_size;

        DescriptorArena m_rtv_heap{};

        UINT m_frame_index = 0;
        sm::UniquePtr<FrameData[]> m_frames;

        static void info_callback(D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID id, LPCSTR desc, void *user);

        void init();

        void query_root_signature_version();

        void setup_device();
        void setup_direct_queue();
        void setup_swapchain();
        void setup_render_targets();
        void setup_direct_allocators();
        void setup_copy_queue();

        void record_commands();
        void finish_commands();
        void submit_commands();

        void await_copy_queue();
        void await_direct_queue();

        sys::Window& get_window() const { return m_config.window; }

    public:
        SM_NOCOPY(Device)

        Device(const RenderConfig& config, Factory& factory);
        ~Device();

        RootSignatureVersion get_root_signature_version() const { return m_version; }
        const RenderConfig& get_config() const { return m_config; }
        DescriptorArena& get_rtv_heap() { return m_rtv_heap; }
        ID3D12Device1 *get_device() const { return m_device.get(); }
        CommandList& get_direct_commands() { return m_direct_list; }
        RhiSink& get_logger() { return m_log; }

        math::uint2 get_swapchain_size() const { return m_swapchain_size; }

        CommandList& open_copy_commands() {
            m_copy_list.reset(m_copy_allocator);
            return m_copy_list;
        }

        CommandList& open_direct_commands(PipelineState& pipeline) {
            auto& alloc = m_frames[m_frame_index].allocator;
            m_direct_list.reset(alloc, pipeline);
            return m_direct_list;
        }

        CommandList& open_direct_commands() {
            auto& alloc = m_frames[m_frame_index].allocator;
            m_direct_list.reset(alloc);
            return m_direct_list;
        }

        void flush_copy_commands(CommandListObject& commands);
        void flush_direct_commands(CommandListObject& commands);

        void resize(math::uint2 size);

        float get_aspect_ratio() const;

        void begin_frame();
        void end_frame();

        void present();
    };

    class Factory {
        friend class Device;

        RenderConfig m_config;
        RhiSink m_log;

        FactoryObject m_factory;
        Object<IDXGIDebug1> m_factory_debug;
        sm::Vector<Adapter> m_adapters;

        void enum_adapters();
        void enum_warp_adapter();
        void init();

        Adapter& get_selected_adapter();

    public:
        SM_NOCOPY(Factory)

        Factory(const RenderConfig& config);
        ~Factory();

        const RenderConfig& get_config() const { return m_config; }

        Device new_device();
    };
}
