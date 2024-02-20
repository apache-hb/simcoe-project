#pragma once

#include "archive/bundle.hpp"
#include "core/array.hpp"
#include "core/bitmap.hpp"
#include "render/instance.hpp"

#include "render/camera.hpp"

#include "core/queue.hpp"

#include <D3D12MemAlloc.h>

namespace sm::render {
    using namespace math;

    using DeviceHandle = Object<ID3D12Device1>;

    constexpr math::float4 kClearColour = { 0.0f, 0.2f, 0.4f, 1.0f };
    constexpr math::float4 kColourBlack = { 0.0f, 0.0f, 0.0f, 1.0f };

    struct RenderConfig {
        DebugFlags flags;
        AdapterPreference preference;
        FeatureLevel feature_level;
        size_t adapter_index;

        uint swapchain_length;
        DXGI_FORMAT swapchain_format;
        math::uint2 swapchain_size; // present resolution

        math::uint2 draw_size; // internal resolution

        uint srv_heap_size;

        Bundle& bundle;
        logs::ILogger &logger;
        sys::Window &window;
    };

    struct FrameData {
        Object<ID3D12Resource> mRenderTarget;
        Object<ID3D12CommandAllocator> mCommandAllocator;
        uint64 mFenceValue;
    };

    struct Resource {
        Object<ID3D12Resource> mResource;
        Object<D3D12MA::Allocation> mAllocation;

        Result map(const D3D12_RANGE *range, void **data);
        void unmap(const D3D12_RANGE *range);

        D3D12_GPU_VIRTUAL_ADDRESS get_gpu_address();
        void reset();
    };

    struct DescriptorHeap : Object<ID3D12DescriptorHeap> {
        uint mDescriptorSize;
        uint mCapacity;

        D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor_handle(int index);
        D3D12_GPU_DESCRIPTOR_HANDLE gpu_descriptor_handle(int index);
    };

    using DescriptorIndex = uint;

    struct DescriptorAllocator {
        DescriptorHeap mHeap;
        sm::BitMap mAllocator;

        void set_heap(DescriptorHeap heap);
        void set_size(uint size) { mAllocator.resize(size); }

        DescriptorIndex allocate();
        void release(DescriptorIndex index);

        D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor_handle(int index);
        D3D12_GPU_DESCRIPTOR_HANDLE gpu_descriptor_handle(int index);

        void reset() { mHeap.reset(); }
        ID3D12DescriptorHeap *get() const { return mHeap.get(); }
    };

    struct Viewport {
        D3D12_VIEWPORT mViewport;
        D3D12_RECT mScissorRect;

        Viewport(math::uint2 size);
    };

    struct Pipeline {
        Object<ID3D12RootSignature> mRootSignature;
        Object<ID3D12PipelineState> mPipelineState;

        void reset();
    };

    class Context {
        static constexpr DXGI_FORMAT kDepthFormat = DXGI_FORMAT_D32_FLOAT;

        const RenderConfig mConfig;

        Sink mSink;
        Instance mInstance;
        WindowState mWindowState;

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

        Result create_resource(Resource& resource, D3D12_HEAP_TYPE heap, D3D12_RESOURCE_DESC desc, D3D12_RESOURCE_STATES state, const D3D12_CLEAR_VALUE *clear = nullptr);

        void serialize_root_signature(Object<ID3D12RootSignature>& signature, const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc);

        /// presentation objects
        Object<IDXGISwapChain3> mSwapChain;

        // render info
        math::uint2 mSwapChainSize; // present resolution
        uint mSwapChainLength; // number of swap chain buffers
        math::uint2 mDrawSize; // render resolution

        sm::UniqueArray<FrameData> mFrames;
        void create_frame_allocators();
        void create_render_targets();

        /// graphics pipeline objects
        Object<ID3D12CommandQueue> mDirectQueue;
        Object<ID3D12GraphicsCommandList1> mCommandList;

        void reset_direct_commands(ID3D12PipelineState *pso = nullptr);

        DescriptorHeap mRtvHeap;
        void create_rtv_heap();
        bool resize_rtv_heap(uint length);

        // +1 for the scene target
        uint min_rtv_heap_size() const { return mSwapChainLength + 1; }
        int scene_rtv_index() const { return 0; }
        int frame_rtv_index(uint frame) const { return int_cast<int>(frame) + 1; }

        // +1 for scene target
        // +1 for imgui font atlas
        uint min_srv_heap_size() const { return 1 + 1 + mConfig.srv_heap_size; }

        DescriptorHeap mDsvHeap;
        DescriptorAllocator mSrvAllocator;

        Resource mDepthStencil;
        void create_depth_stencil();


        Result create_descriptor_heap(DescriptorHeap& heap, D3D12_DESCRIPTOR_HEAP_TYPE type, uint capacity, bool shader_visible);

        /// copy queue and commands
        Object<ID3D12CommandQueue> mCopyQueue;
        Object<ID3D12CommandAllocator> mCopyAllocator;
        Object<ID3D12GraphicsCommandList1> mCopyCommands;
        void create_copy_queue();
        void reset_copy_commands();

        Object<ID3D12Fence> mCopyFence;
        HANDLE mCopyFenceEvent;
        uint64 mCopyFenceValue;
        void create_copy_fence();


        DescriptorIndex mSceneTargetSrvIndex;

        /// blit pipeline + assets
        Viewport mPresentViewport;
        Pipeline mBlitPipeline;

        struct {
            Resource mVertexBuffer;
            D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
        } mScreenQuad;

        void update_display_viewport();

        void create_blit_pipeline();
        void destroy_blit_pipeline();

        void create_screen_quad();
        void destroy_screen_quad();



        /// scene pipeline + assets
        Viewport mSceneViewport;
        void update_scene_viewport();

        Pipeline mPrimitivePipeline;
        void create_primitive_pipeline();
        void destroy_primitive_pipeline();

        Resource mSceneTarget;
        void create_scene_target();
        void destroy_scene_target();
        void create_scene_render_target();

        struct Primitive {
            draw::MeshInfo mInfo;
            draw::Transform mTransform;

            Resource mVertexBuffer;
            D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;

            Resource mIndexBuffer;
            D3D12_INDEX_BUFFER_VIEW mIndexBufferView;

            uint32 mIndexCount;
        };

        sm::Vector<Primitive> mPrimitives;

        struct Upload {
            Resource resource;
            uint64 value;

            constexpr auto operator<=>(const Upload& other) const { return value <=> other.value; }
        };

        sm::PriorityQueue<Upload, std::greater<Upload>> mUploads;

        Primitive create_mesh(const draw::MeshInfo& info, const float3& colour);

        void init_scene();
        void create_scene();
        void destroy_scene();

        void create_pipeline();
        void create_assets();

        uint mFrameIndex;
        HANDLE mFenceEvent;
        Object<ID3D12Fence> mFence;

        void copy_buffer(Object<ID3D12GraphicsCommandList1>& list, Resource& dst, Resource& src, size_t size);

        /// synchronization

        void build_command_list();
        void move_to_next_frame();
        void wait_for_gpu();
        void flush_copy_queue();

        /// dear imgui

        draw::MeshInfo mMeshCreateInfo[draw::MeshType::kCount];
        DescriptorIndex mImGuiSrvIndex;

        void create_imgui();
        void destroy_imgui();

        void create_imgui_backend();
        void destroy_imgui_backend();

        bool update_imgui();
        void render_imgui();

        /// state updates

        void update_adapter(size_t index);
        void update_swapchain_length(uint length);


        void destroy_device();

    public:
        Context(const RenderConfig& config);

        void create();
        void destroy();

        bool update();
        void render();
        void resize_draw(math::uint2 size);
        void resize_swapchain(math::uint2 size);

        draw::Camera camera;
    };
}
