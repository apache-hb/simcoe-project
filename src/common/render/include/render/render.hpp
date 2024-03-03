#pragma once

#include "archive/bundle.hpp"

#include "core/array.hpp"

#include "render/instance.hpp"
#include "render/heap.hpp"
#include "render/commands.hpp"
#include "render/camera.hpp"
#include "render/resource.hpp"

#include "render/graph.hpp"

namespace sm::render {
    using namespace math;

    using DeviceHandle = Object<ID3D12Device1>;

    using VertexBufferView = D3D12_VERTEX_BUFFER_VIEW;
    using IndexBufferView = D3D12_INDEX_BUFFER_VIEW;

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

        DXGI_FORMAT scene_format;
        DXGI_FORMAT depth_format;
        math::uint2 scene_size; // internal resolution

        uint rtv_heap_size;
        uint dsv_heap_size;
        uint srv_heap_size;

        Bundle& bundle;
        logs::ILogger &logger;
        sys::Window &window;
    };

    struct FrameData {
        Object<ID3D12Resource> target;
        Object<ID3D12CommandAllocator> allocator;
        RtvIndex rtv_index;
        uint64 fence_value;
    };

    struct Viewport {
        D3D12_VIEWPORT mViewport;
        D3D12_RECT mScissorRect;

        Viewport(math::uint2 size);
    };

    struct Pipeline {
        Object<ID3D12RootSignature> signature;
        Object<ID3D12PipelineState> pso;

        void reset();
    };

    struct Texture {
        Object<ID3D12Resource> mResource;
        SrvIndex mSrvIndex;

        sm::Vector<D3D12_SUBRESOURCE_DATA> mMipData;
    };

    struct Context {
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

        Result load_dds_texture(Object<ID3D12Resource>& texture, sm::Vector<D3D12_SUBRESOURCE_DATA>& mips, const char *name);

        void serialize_root_signature(Object<ID3D12RootSignature>& signature, const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc);

        /// presentation objects
        Object<IDXGISwapChain3> mSwapChain;

        // render info
        DXGI_FORMAT mSwapChainFormat;
        math::uint2 mSwapChainSize; // present resolution
        uint mSwapChainLength; // number of swap chain buffers

        DXGI_FORMAT mSceneFormat;
        DXGI_FORMAT mDepthFormat;
        math::uint2 mSceneSize; // render resolution

        sm::UniqueArray<FrameData> mFrames;
        void create_frame_allocators();
        void create_frame_rtvs();
        void destroy_frame_rtvs();
        void create_render_targets();

        /// graphics pipeline objects
        Object<ID3D12CommandQueue> mDirectQueue;
        CommandList mCommandList;

        void reset_direct_commands(ID3D12PipelineState *pso = nullptr);

        uint min_rtv_heap_size() const { return DXGI_MAX_SWAP_CHAIN_BUFFERS + 1 + mConfig.rtv_heap_size; }
        uint min_srv_heap_size() const { return 1 + 1 + mConfig.srv_heap_size; }
        uint min_dsv_heap_size() const { return 1 + mConfig.dsv_heap_size; }

        RtvPool mRtvPool;
        DsvPool mDsvPool;
        SrvPool mSrvPool;

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

        /// blit pipeline + assets
        Viewport mPresentViewport;
        Pipeline mBlitPipeline;

        Texture mTexture;
        sm::Vector<Texture> mTextures;

        struct {
            Resource mVertexBuffer;
            VertexBufferView mVertexBufferView;
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

        struct Primitive {
            draw::MeshInfo mInfo;
            draw::Transform mTransform;

            Resource mVertexBuffer;
            VertexBufferView mVertexBufferView;

            Resource mIndexBuffer;
            IndexBufferView mIndexBufferView;

            uint32 mIndexCount;
        };

        sm::Vector<Primitive> mPrimitives;

        Primitive create_mesh(const draw::MeshInfo& info, const float3& colour);

        void init_scene();
        void create_scene();
        void destroy_scene();

        void create_frame_graph();
        void destroy_frame_graph();

        void create_pipeline();
        void create_assets();

        uint mFrameIndex;
        HANDLE mFenceEvent;
        Object<ID3D12Fence> mFence;

        void copy_buffer(Object<ID3D12GraphicsCommandList1>& list, Resource& dst, Resource& src, size_t size);

        // render graph
        graph::Handle mSwapChainHandle;
        graph::Handle mSceneTargetHandle;
        graph::FrameGraph mFrameGraph;

        /// synchronization

        void build_command_list();
        void move_to_next_frame();
        void wait_for_gpu();
        void flush_copy_queue();

        /// dear imgui

        draw::MeshInfo mMeshCreateInfo[draw::MeshType::kCount];
        SrvIndex mImGuiSrvIndex;

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
