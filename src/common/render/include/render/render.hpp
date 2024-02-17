#pragma once

#include "core/array.hpp"
#include "render/instance.hpp"

#include <D3D12MemAlloc.h>

namespace sm::render {
    using DeviceHandle = Object<ID3D12Device1>;

    struct RenderConfig {
        DebugFlags flags;
        AdapterPreference preference;
        FeatureLevel feature_level;
        size_t adapter_index;

        uint swapchain_length;
        DXGI_FORMAT swapchain_format;
        math::uint2 swapchain_size;

        logs::ILogger &logger;
        sys::Window &window;
    };

    struct FrameData {
        Object<ID3D12Resource> mRenderTarget;
        Object<ID3D12CommandAllocator> mCommandAllocator;
        uint64 mFenceValue;
    };

    struct Resource {
        Object<ID3D12Resource> mHandle;
        Object<D3D12MA::Allocation> mAllocation;

        Result map(const D3D12_RANGE *range, void **data);
        void unmap(const D3D12_RANGE *range);

        D3D12_GPU_VIRTUAL_ADDRESS get_gpu_address();
        void reset();
    };

    struct Context {
        RenderConfig mConfig;

        Sink mSink;
        Instance mInstance;

        static void CALLBACK on_info(D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity,
                                 D3D12_MESSAGE_ID id, LPCSTR desc, void *user);

        /// device creation and physical adapters
        size_t mAdapterIndex;
        Adapter *mAdapter = nullptr;
        DeviceHandle mDevice;
        Object<ID3D12Debug1> mDebug;
        Object<ID3D12InfoQueue1> mInfoQueue;
        DWORD mCookie = ULONG_MAX;

        FeatureLevel get_feature_level() const { return mConfig.feature_level; }

        void enable_debug_layer(bool gbv, bool rename);
        void enable_dred();
        void enable_info_queue();
        void create_device();

        // resource allocator
        Object<D3D12MA::Allocator> mAllocator;
        void create_allocator();

        Result create_resource(Resource& resource, D3D12_HEAP_TYPE heap, D3D12_RESOURCE_DESC desc, D3D12_RESOURCE_STATES state);

        /// presentation objects
        Object<IDXGISwapChain3> mSwapChain;
        math::uint2 mSwapChainSize;

        sm::UniqueArray<FrameData> mFrames;
        void create_frame_allocators();

        /// graphics pipeline objects
        Object<ID3D12CommandQueue> mDirectQueue;
        Object<ID3D12RootSignature> mRootSignature;
        Object<ID3D12PipelineState> mPipelineState;
        Object<ID3D12GraphicsCommandList1> mCommandList;

        Object<ID3D12DescriptorHeap> mRtvHeap;
        uint mRtvDescriptorSize = 0;

        Object<ID3D12DescriptorHeap> mSrvHeap;
        uint mSrvDescriptorSize = 0;
        void create_pipeline_state();

        /// copy queue and commands
        Object<ID3D12CommandQueue> mCopyQueue;
        Object<ID3D12CommandAllocator> mCopyAllocator;
        Object<ID3D12GraphicsCommandList1> mCopyCommands;
        void create_copy_queue();

        Object<ID3D12Fence> mCopyFence;
        HANDLE mCopyFenceEvent;
        uint64 mCopyFenceValue;
        void create_copy_fence();

        // scissor + viewport
        D3D12_VIEWPORT mViewport;
        D3D12_RECT mScissorRect;
        void update_viewport_scissor();

        // assets
        Resource mVertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
        void create_triangle();

        void create_size_dependent_resources();

        void create_pipeline();
        void create_assets();

        uint mFrameIndex;
        HANDLE mFenceEvent;
        Object<ID3D12Fence> mFence;

        void build_command_list();
        void move_to_next_frame();
        void wait_for_gpu();
        void flush_copy_queue();

        void create_imgui();
        void destroy_imgui();

        void update_imgui();
        void render_imgui();

    public:
        Context(const RenderConfig& config);

        void create();
        void destroy();

        void update();
        void render();
        void resize(math::uint2 size);
    };
}
