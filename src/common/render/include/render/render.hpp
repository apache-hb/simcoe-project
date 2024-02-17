#pragma once

#include "core/array.hpp"
#include "render/instance.hpp"

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

        /// presentation objects
        Object<IDXGISwapChain3> mSwapChain;
        math::uint2 mSwapChainSize;
        sm::UniqueArray<Object<ID3D12Resource>> mRenderTargets;
        void create_render_targets();

        sm::UniqueArray<Object<ID3D12CommandAllocator>> mAllocators;
        sm::UniqueArray<uint64> mFenceValues;
        void create_frame_objects();

        /// graphics pipeline objects
        Object<ID3D12CommandQueue> mQueue;
        Object<ID3D12RootSignature> mRootSignature;
        Object<ID3D12PipelineState> mPipelineState;
        Object<ID3D12GraphicsCommandList1> mCommandList;
        Object<ID3D12DescriptorHeap> mRtvHeap;
        uint mRtvDescriptorSize = 0;

        void create_pipeline_state();
        void create_triangle();

        // scissor + viewport
        D3D12_VIEWPORT mViewport;
        D3D12_RECT mScissorRect;

        void update_viewport_scissor();

        // assets
        Object<ID3D12Resource> mVertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;

        void create_size_dependent_resources();
        void create_pipeline();
        void create_assets();

        uint mFrameIndex;
        HANDLE mFenceEvent;
        Object<ID3D12Fence> mFence;

        void build_command_list();
        void move_to_next_frame();
        void wait_for_gpu();

    public:
        Context(const RenderConfig& config);

        void create();
        void destroy();

        void update();
        void render();
        void resize(math::uint2 size);
    };
}
