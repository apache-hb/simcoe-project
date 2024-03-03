#pragma once

#include "archive/bundle.hpp"
#include "core/array.hpp"
#include "core/bitmap.hpp"

#include "render/instance.hpp"
#include "render/gpu_objects.hpp"
#include "render/camera.hpp"

// #include "core/queue.hpp"

#include <D3D12MemAlloc.h>

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

    struct Resource {
        Object<ID3D12Resource> mResource;
        Object<D3D12MA::Allocation> mAllocation;

        Result map(const D3D12_RANGE *range, void **data);
        void unmap(const D3D12_RANGE *range);

        D3D12_GPU_VIRTUAL_ADDRESS get_gpu_address();
        void reset();

        ID3D12Resource *get() const { return mResource.get(); }
    };

    template<DescriptorType::Inner D>
    class DescriptorPool {
        Object<ID3D12DescriptorHeap> mHeap;
        uint mDescriptorSize;
        uint mCapacity;
        sm::BitMap mAllocator;
    public:
        constexpr static DescriptorType kType = D;

        enum class Index : uint { eInvalid = UINT_MAX };

        bool test_bit(Index index) const { return mAllocator.test(sm::BitMap::Index{size_t(index)}); }

        Result init(ID3D12Device1 *device, uint capacity, D3D12_DESCRIPTOR_HEAP_FLAGS flags) {
            constexpr D3D12_DESCRIPTOR_HEAP_TYPE kHeapType = kType.as_facade();
            const D3D12_DESCRIPTOR_HEAP_DESC kDesc = {
                .Type = kHeapType,
                .NumDescriptors = capacity,
                .Flags = flags,
                .NodeMask = 0,
            };

            mDescriptorSize = device->GetDescriptorHandleIncrementSize(kHeapType);
            mCapacity = capacity;
            mAllocator.resize(capacity);

            return device->CreateDescriptorHeap(&kDesc, IID_PPV_ARGS(&mHeap));
        }

        Index allocate() {
            auto index = mAllocator.allocate();
            CTASSERTF(index < mCapacity, "DescriptorPool: allocate out of range. index %u capacity %u", enum_cast<uint>(index), mCapacity);
            CTASSERTF(index != sm::BitMap::eInvalid, "DescriptorPool: allocate out of descriptors. size %u", mCapacity);
            return Index{uint(index)};
        }

        void release(Index index) {
            mAllocator.release(sm::BitMap::Index{size_t(index)});
        }

        D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle(Index index) {
            CTASSERTF(test_bit(index), "DescriptorPool: cpu_handle index %u is not set", enum_cast<uint>(index));
            D3D12_CPU_DESCRIPTOR_HANDLE handle = mHeap->GetCPUDescriptorHandleForHeapStart();
            handle.ptr += enum_cast<uint>(index) * mDescriptorSize;
            return handle;
        }

        D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle(Index index) {
            CTASSERTF(test_bit(index), "DescriptorPool: gpu_handle index %u is not set", enum_cast<uint>(index));
            D3D12_GPU_DESCRIPTOR_HANDLE handle = mHeap->GetGPUDescriptorHandleForHeapStart();
            handle.ptr += enum_cast<uint>(index) * mDescriptorSize;
            return handle;
        }

        void reset() { mHeap.reset(); mAllocator.reset(); }
        ID3D12DescriptorHeap *get() const { return mHeap.get(); }

        uint get_capacity() const { return mCapacity; }
    };

    using RtvPool = DescriptorPool<DescriptorType::eRTV>;
    using DsvPool = DescriptorPool<DescriptorType::eDSV>;
    using SrvPool = DescriptorPool<DescriptorType::eSRV>;

    using RtvIndex = RtvPool::Index;
    using DsvIndex = DsvPool::Index;
    using SrvIndex = SrvPool::Index;

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
        Object<ID3D12RootSignature> mRootSignature;
        Object<ID3D12PipelineState> mPipelineState;

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
        Object<ID3D12GraphicsCommandList1> mCommandList;

        void reset_direct_commands(ID3D12PipelineState *pso = nullptr);

        uint min_rtv_heap_size() const { return DXGI_MAX_SWAP_CHAIN_BUFFERS + 1 + mConfig.rtv_heap_size; }
        uint min_srv_heap_size() const { return 1 + 1 + mConfig.srv_heap_size; }
        uint min_dsv_heap_size() const { return 1 + mConfig.dsv_heap_size; }

        RtvPool mRtvPool;
        DsvPool mDsvPool;
        SrvPool mSrvPool;

        DsvIndex mDepthStencilIndex;
        Resource mDepthStencil;
        void create_depth_stencil();
        void destroy_depth_stencil();

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

        SrvIndex mSceneTargetSrvIndex;
        RtvIndex mSceneTargetRtvIndex;

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
        void create_scene_rtv();
        void destroy_scene_rtv();

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
