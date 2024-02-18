#pragma once

#include "core/array.hpp"
#include "render/instance.hpp"

#include <D3D12MemAlloc.h>

namespace sm::render {
    using namespace math;

    using DeviceHandle = Object<ID3D12Device1>;

    constexpr float3 kVectorForward = {1.f, 0.f, 0.f};
    constexpr float3 kVectorRight = {0.f, 1.f, 0.f};
    constexpr float3 kVectorUp = {0.f, 0.f, 1.f};

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
        const RenderConfig mConfig;

        Sink mSink;
        Instance mInstance;

        static void CALLBACK on_info(D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity,
                                 D3D12_MESSAGE_ID id, LPCSTR desc, void *user);

        /// device creation and physical adapters
        size_t mAdapterIndex;
        DeviceHandle mDevice;
        RootSignatureVersion mRootSignatureVersion;
        Object<ID3D12Debug1> mDebug;
        Object<ID3D12InfoQueue1> mInfoQueue;
        DWORD mCookie = ULONG_MAX;

        FeatureLevel get_feature_level() const { return mConfig.feature_level; }

        void enable_debug_layer(bool gbv, bool rename);
        void enable_dred();
        void enable_info_queue();
        void query_root_signature_version();
        void create_device(size_t adapter);

        // resource allocator
        Object<D3D12MA::Allocator> mAllocator;
        void create_allocator();

        Result create_resource(Resource& resource, D3D12_HEAP_TYPE heap, D3D12_RESOURCE_DESC desc, D3D12_RESOURCE_STATES state);

        void serialize_root_signature(Object<ID3D12RootSignature>& signature, const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc);

        /// presentation objects
        Object<IDXGISwapChain3> mSwapChain;
        math::uint2 mSwapChainSize;
        uint mSwapChainLength;

        sm::UniqueArray<FrameData> mFrames;
        void create_frame_allocators();
        void create_render_targets();

        /// graphics pipeline objects
        Object<ID3D12CommandQueue> mDirectQueue;
        Object<ID3D12GraphicsCommandList1> mCommandList;

        Object<ID3D12DescriptorHeap> mRtvHeap;
        uint mRtvDescriptorSize = 0;
        void create_rtv_heap(uint count);

        Object<ID3D12DescriptorHeap> mSrvHeap;
        uint mSrvDescriptorSize = 0;

        Object<ID3D12RootSignature> mRootSignature;
        Object<ID3D12PipelineState> mPipelineState;
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

        struct {
            Object<ID3D12RootSignature> mRootSignature;
            Object<ID3D12PipelineState> mPipelineState;
        } mPrimitive;

        void create_primitive_pipeline();
        void destroy_primitive_pipeline();

        struct Primitive {
            Resource mVertexBuffer;
            D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;

            Resource mIndexBuffer;
            D3D12_INDEX_BUFFER_VIEW mIndexBufferView;

            uint32 mIndexCount;
        } mCube;

        void create_cube();
        void destroy_cube();


        struct Camera {
            float3 position = {3.f, 0.f, 0.f};
            float3 direction = kVectorForward;

            float fov = to_radians(75.f);
            float speed = 3.f;

            float4x4 model() const;
            float4x4 view() const;
            float4x4 projection(float aspect) const;

            float4x4 mvp(float aspect) const;
        } mCamera;

        void update_camera();

        void create_pipeline();
        void create_assets();

        uint mFrameIndex;
        HANDLE mFenceEvent;
        Object<ID3D12Fence> mFence;

        void copy_buffer(Object<ID3D12GraphicsCommandList1>& list, Resource& dst, Resource& src, size_t size);

        void build_command_list();
        void move_to_next_frame();
        void wait_for_gpu();
        void flush_copy_queue();

        void create_imgui();
        void destroy_imgui();

        void create_imgui_device();
        void destroy_imgui_device();

        void update_imgui();
        void render_imgui();

        void update_adapter(size_t index);
        void update_swapchain_length(uint length);

        void destroy_device();

    public:
        Context(const RenderConfig& config);

        void create();
        void destroy();

        void update();
        void render();
        void resize(math::uint2 size);
    };
}
