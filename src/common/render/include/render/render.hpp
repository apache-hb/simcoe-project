#pragma once

#include "archive/bundle.hpp"

#include "core/array.hpp"

#include "render/instance.hpp"
#include "render/heap.hpp"
#include "render/resource.hpp"
#include "render/dstorage.hpp"

#include "render/graph.hpp"

#include "world/world.hpp"

#include <directx/d3dx12_core.h>
#include <directx/d3dx12_check_feature_support.h>

namespace sm::render {
    using namespace math;

    using DeviceHandle = Object<ID3D12Device1>;

    using VertexBufferView = D3D12_VERTEX_BUFFER_VIEW;
    using IndexBufferView = D3D12_INDEX_BUFFER_VIEW;

    constexpr math::float4 kClearColour = { 0.0f, 0.2f, 0.4f, 1.0f };
    constexpr math::float4 kColourBlack = { 0.0f, 0.0f, 0.0f, 1.0f };

    struct SwapChainConfig {
        math::uint2 size;
        uint length;
        DXGI_FORMAT format;
    };

    struct RenderConfig {
        DebugFlags flags;
        AdapterPreference preference;
        FeatureLevel feature_level;
        LUID adapter_luid = { 0, 0 };

        SwapChainConfig swapchain;

        uint rtv_heap_size;
        uint dsv_heap_size;
        uint srv_heap_size;

        uint max_lights = 4096;

        Bundle& bundle;
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

        Viewport() = default;
        Viewport(math::uint2 size);
        Viewport(D3D12_VIEWPORT viewport, D3D12_RECT scissor)
            : mViewport(viewport)
            , mScissorRect(scissor)
        { }

        static Viewport letterbox(math::uint2 display, math::uint2 render);
    };

    struct Pipeline {
        Object<ID3D12RootSignature> signature;
        Object<ID3D12PipelineState> pso;

        void reset();
    };

    struct MeshResource {
        Resource vbo;
        VertexBufferView vbo_view;

        Resource ibo;
        IndexBufferView ibo_view;

        uint32 index_count;
    };

    struct TextureResource {
        Resource resource;
        SrvIndex srv;
    };

    struct PointLight {
        math::float3 position;
        math::float3 colour;
        float intensity;
    };

    struct Context {
        const RenderConfig mConfig;

        DebugFlags mDebugFlags;

        Instance mInstance;
        WindowState mWindowState;

        static void CALLBACK on_info(
            D3D12_MESSAGE_CATEGORY category,
            D3D12_MESSAGE_SEVERITY severity,
            D3D12_MESSAGE_ID id,
            LPCSTR desc,
            void *user);

        /// device creation and physical adapters
        Adapter *mCurrentAdapter;
        DeviceHandle mDevice;
        CD3DX12FeatureSupport mFeatureSupport;
        RootSignatureVersion mRootSignatureVersion;
        Object<ID3D12Debug1> mDebug;
        Object<ID3D12InfoQueue1> mInfoQueue;
        DWORD mCookie = ULONG_MAX;

        void set_current_adapter(Adapter& adapter) { mCurrentAdapter = std::addressof(adapter); }

        void enable_debug_layer(bool gbv, bool rename);
        void disable_debug_layer();
        void enable_dred(bool enabled);
        void enable_info_queue();
        void query_root_signature_version();
        void create_device(Adapter& adapter);

        // resource allocator
        Object<D3D12MA::Allocator> mAllocator;
        void create_allocator();

        Result create_resource(Resource& resource, D3D12_HEAP_TYPE heap, D3D12_RESOURCE_DESC desc, D3D12_RESOURCE_STATES state, const D3D12_CLEAR_VALUE *clear = nullptr);

        void serialize_root_signature(Object<ID3D12RootSignature>& signature, const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc);

        /// presentation objects
        Object<IDXGISwapChain3> mSwapChain;

        // render info
        SwapChainConfig mSwapChainConfig;

        sm::UniqueArray<FrameData> mFrames;
        void create_frame_allocators();
        void create_frame_rtvs();
        void destroy_frame_rtvs();
        void create_render_targets();

        /// TODO: remove this command list, should be created elsewhere

        /// graphics pipeline objects
        Object<ID3D12CommandQueue> mDirectQueue;
        Object<ID3D12GraphicsCommandList1> mCommandList;

        void reset_direct_commands(ID3D12PipelineState *pso = nullptr);

        uint min_rtv_heap_size() const { return DXGI_MAX_SWAP_CHAIN_BUFFERS + mConfig.rtv_heap_size; }
        uint min_srv_heap_size() const { return mConfig.srv_heap_size; }
        uint min_dsv_heap_size() const { return mConfig.dsv_heap_size; }

        RtvPool mRtvPool;
        DsvPool mDsvPool;
        SrvPool mSrvPool;

        /// compute queue and commands
        Object<ID3D12CommandQueue> mComputeQueue;
        Object<ID3D12GraphicsCommandList1> mComputeCommands;

        void create_compute_queue();
        void destroy_compute_queue();

        /// device sync fence
        Object<ID3D12Fence> mDeviceFence;
        uint mDeviceFenceValue;

        void create_device_fence();
        void destroy_device_fence();

        /// copy queue and commands
        Object<ID3D12CommandQueue> mCopyQueue;
        Object<ID3D12CommandAllocator> mCopyAllocator;
        Object<ID3D12GraphicsCommandList1> mCopyCommands;
        void create_copy_queue();
        void destroy_copy_queue();

        void reset_copy_commands();

        Object<ID3D12Fence> mCopyFence;
        HANDLE mCopyFenceEvent;
        uint64 mCopyFenceValue;
        void create_copy_fence();

        ID3D12CommandQueue *get_queue(CommandListType type);

        struct {
            Resource upload;
            void *mapped;
            sm::Span<PointLight> lights;
            uint count;

            Resource resource;
            SrvIndex srv;
        } mPointLights;

        void create_lights();
        void destroy_lights();

        void update_node_lights(world::IndexOf<world::Node> node, uint& index, const float4x4& parent);

        void update_lights(ID3D12GraphicsCommandList1 *list);

        // world data
        world::World mWorld;
        world::IndexOf<world::Scene> mCurrentScene = world::kInvalidIndex;
        sm::HashMap<world::IndexOf<world::Model>, MeshResource> mMeshes;
        sm::HashMap<world::IndexOf<world::Image>, TextureResource> mImages;

        void load_image(world::IndexOf<world::Image> index);

        void load_object(world::IndexOf<world::Model> index, const world::Object& object);
        void load_mesh_buffer(world::IndexOf<world::Model> index, const world::Mesh& mesh);
        void load_buffer_view(world::IndexOf<world::Model> index, const world::BufferView& view);

        void create_node(world::IndexOf<world::Node> node);
        void create_model(world::IndexOf<world::Model> model);

        void begin_upload();
        void end_upload();

        void upload(auto&& fn) {
            begin_upload();
            fn();
            end_upload();
        }

        void init_scene();

        void load_scene();
        void reset_scene();

        void create_scene();
        void destroy_scene();

        void create_framegraph();
        void destroy_framegraph();

        void create_pipeline();
        void create_assets();

        void destroy_textures();
        void create_textures();

        uint mFrameIndex;
        HANDLE mFenceEvent;
        Object<ID3D12Fence> mFence;

        void copy_buffer(ID3D12GraphicsCommandList1 *list, Resource& dst, Resource& src, size_t size);

        // dstorage
        size_t mStorageMeshCount = 0;
        CopyStorage mStorage;
        sm::HashMap<world::IndexOf<world::File>, Object<IDStorageFile>> mStorageFiles;
        sm::HashMap<size_t, world::Mesh> mStorageMeshes;
        void create_dstorage();
        void destroy_dstorage();

        IDStorageFile *get_storage_file(world::IndexOf<world::File> index);
        const uint8 *get_storage_buffer(world::IndexOf<world::Buffer> index);

        void upload_buffer_view(RequestBuilder& request, const world::BufferView& view);

        // render graph
        graph::Handle mSwapChainHandle;
        graph::FrameGraph mFrameGraph;
        /// synchronization
        void build_command_list();
        void move_to_next_frame();
        void wait_for_gpu();
        void flush_copy_queue();

        /// state updates
        void update_adapter(Adapter& adapter);
        void update_swapchain_length(uint length);
        void resize_draw(math::uint2 size);

    private:
        void destroy_device();

        virtual void on_create() { }
        virtual void on_destroy() { }
        virtual void setup_framegraph(graph::FrameGraph& graph) = 0;

    public:
        virtual ~Context() = default;
        Context(const RenderConfig& config);

        void create();
        void destroy();

        void render();
        void resize_swapchain(math::uint2 size);
        void recreate_device();
        void update_framegraph();

        world::IndexOf<world::Scene> get_current_scene() const { return mCurrentScene; }
        void set_scene(world::IndexOf<world::Scene> scene);

        world::Scene& get_scene() { return mWorld.get(mCurrentScene); }
        FeatureLevel get_feature_level() const { return mConfig.feature_level; }

        Adapter& get_current_adapter() { return *mCurrentAdapter; }

        uint64 get_image_footprint(
            world::IndexOf<world::Image> image,
            sm::Span<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints);

        template<typename T>
        ConstBuffer<T> cbuffer() {
            // round up the size to the next multiple of 256
            size_t size = (sizeof(T) + 255) & ~255;
            auto desc = CD3DX12_RESOURCE_DESC::Buffer(size);

            ConstBuffer<T> buffer;
            SM_ASSERT_HR(create_resource(buffer, D3D12_HEAP_TYPE_UPLOAD, desc, D3D12_RESOURCE_STATE_GENERIC_READ));

            buffer.init();

            return buffer;
        }

        template<typename T>
        VertexBuffer<T> vertex_upload_buffer(size_t count) {
            auto desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(T) * count);

            VertexBuffer<T> buffer;
            SM_ASSERT_HR(create_resource(buffer, D3D12_HEAP_TYPE_UPLOAD, desc, D3D12_RESOURCE_STATE_GENERIC_READ));

            buffer.init(count);

            return buffer;
        }
    };
}
