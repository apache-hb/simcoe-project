#pragma once

#include "core/bitmap.hpp"
#include "core/core.hpp"
#include "core/map.hpp"
#include "core/queue.hpp"
#include "core/span.hpp"
#include "core/vector.hpp"

#include "logs/sink.hpp"

#include "render/object.hpp"

#include <D3D12MemAlloc.h>

#include "render.reflect.h"

namespace sm::render {
class Context;

using Sink = logs::Sink<logs::Category::eRender>;

using DeviceHandle = Object<ID3D12Device1>;

struct InstanceConfig {
    DebugFlags flags;
    AdapterPreference preference;
    logs::ILogger &logger;
};

class Adapter : public Object<IDXGIAdapter1> {
    sm::String mName;
    sm::Memory mVideoMemory{0};
    sm::Memory mSystemMemory{0};
    sm::Memory mSharedMemory{0};
    AdapterFlag mFlags;

public:
    using Object::Object;

    Adapter(IDXGIAdapter1 *adapter);

    constexpr std::string_view name() const {
        return mName;
    }

    constexpr sm::Memory vidmem() const {
        return mVideoMemory;
    }
    constexpr sm::Memory sysmem() const {
        return mSystemMemory;
    }
    constexpr sm::Memory sharedmem() const {
        return mSharedMemory;
    }

    constexpr AdapterFlag flags() const {
        return mFlags;
    }
};

class Instance {
    Sink mSink;

    DebugFlags mFlags;
    AdapterPreference mAdapterSearch;

    Object<IDXGIFactory4> mFactory;
    Object<IDXGIDebug1> mDebug;
    sm::Vector<Adapter> mAdapters;
    Adapter mWarpAdapter;

    void enable_leak_tracking();

public:
    Instance(InstanceConfig config);
    ~Instance();

    bool enum_by_preference();
    void enum_warp_adapter();
    void enum_adapters();

    Adapter &warp_adapter() {
        return mWarpAdapter;
    }
    Adapter &get_adapter(size_t index) {
        return mAdapters[index];
    }

    Object<IDXGIFactory4> &factory() {
        return mFactory;
    }

    const DebugFlags &flags() const {
        return mFlags;
    }
};

struct DeviceResource {
    Object<ID3D12Resource> resource;
    Object<D3D12MA::Allocation> allocation;

    void write(const void *data, size_t size);

    template<StandardLayout T>
    void write(sm::Span<const T> data) {
        write(data.data(), data.size_bytes());
    }

    ID3D12Resource *get() const {
        return resource.get();
    }

    GpuAddress gpu_address() {
        return GpuAddress(resource->GetGPUVirtualAddress());
    }
};

VertexBufferView vbo_view(DeviceResource &resource, uint stride, uint size);
IndexBufferView ibo_view(DeviceResource &resource, uint size, DataFormat type);

struct DescriptorRange {
    DescriptorRangeType type;
    uint count;
    uint base;
    uint space;
    uint offset;
};

struct RootData {
    uint reg;
    uint space;
    uint count;
};

struct RootParameter {
    RootParameterType type;
    ShaderVisibility visibility;

    union {
        sm::Span<const DescriptorRange> ranges;
        RootData root;
    };

    static constexpr RootParameter table(sm::Span<const DescriptorRange> table,
                                         ShaderVisibility visibility) {
        return {
            .type = RootParameterType::eTable,
            .visibility = visibility,
            .ranges = table,
        };
    }

    static constexpr RootParameter consts(uint reg, uint space, uint count,
                                          ShaderVisibility visibility) {
        return {
            .type = RootParameterType::eRootData,
            .visibility = visibility,
            .root = {reg, space, count},
        };
    }

    static constexpr RootParameter cbv(uint reg, uint space, uint count,
                                       ShaderVisibility visibility) {
        return {
            .type = RootParameterType::eConstBuffer,
            .visibility = visibility,
            .root = {reg, space, count},
        };
    }

    static constexpr RootParameter srv(uint reg, uint space, uint count,
                                       ShaderVisibility visibility) {
        return {
            .type = RootParameterType::eShaderResource,
            .visibility = visibility,
            .root = {reg, space, count},
        };
    }

    static constexpr RootParameter uav(uint reg, uint space, uint count,
                                       ShaderVisibility visibility) {
        return {
            .type = RootParameterType::eUnorderedAccess,
            .visibility = visibility,
            .root = {reg, space, count},
        };
    }
};

struct Sampler {
    Filter filter;
    AddressMode address;
    uint reg;
    uint space;
    ShaderVisibility visibility;
};

struct ShaderBytecode {
    sm::Span<const uint8> data;
};

struct PipelineConfig {
    RootSignatureFlags flags;
    sm::Span<const InputElement> input;
    sm::Span<const RootParameter> params;
    sm::Span<const Sampler> samplers;

    ShaderBytecode vs;
    ShaderBytecode ps;
};

class PipelineState {
    Object<ID3D12RootSignature> mSignature;
    Object<ID3D12PipelineState> mState;

public:
    PipelineState() = default;

    void create(DeviceHandle &device, const PipelineConfig &config);

    ID3D12PipelineState *pso() const {
        return mState.get();
    }
    ID3D12RootSignature *signature() const {
        return mSignature.get();
    }
};

using Barrier = D3D12_RESOURCE_BARRIER;

Barrier transition_barrier(DeviceResource &resource, ResourceState before,
                        ResourceState after);

struct CommandList {
    Object<ID3D12CommandAllocator> allocator;
    Object<ID3D12GraphicsCommandList> list;

    void create(DeviceHandle &device, CommandListType type);

    void bind_pipeline(const PipelineState &state) {
        list->SetPipelineState(state.pso());
        list->SetGraphicsRootSignature(state.signature());
    }

    bool is_valid() const {
        return list.is_valid();
    }

    void close();
    void reset();

    void copy_buffer(DeviceResource& src, DeviceResource& dst, uint size);

    void barriers(sm::Span<const Barrier> barriers) {
        list->ResourceBarrier(uint(barriers.size()), barriers.data());
    }

    ID3D12GraphicsCommandList *get() const {
        return list.get();
    }
};

constexpr auto kCloseHandle = [](HANDLE handle) { CloseHandle(handle); };

struct Fence {
    Object<ID3D12Fence> fence;
    sm::UniqueHandle<HANDLE, decltype(kCloseHandle)> event;

    void create(DeviceHandle &device, const char *name = nullptr);

    void wait(uint64 pending);
    uint64 value() {
        return fence->GetCompletedValue();
    }
};

struct CommandQueue {
    Object<ID3D12CommandQueue> queue;

    void create(DeviceHandle &device, CommandListType type);
    void signal(Fence &fence, uint64 value);
    void execute(uint count, ID3D12CommandList *const *lists);

    ID3D12CommandQueue *get() const {
        return queue.get();
    }
};

enum DescriptorIndex : uint {
    eInvalid = UINT_MAX
};

class DescriptorArena : Object<ID3D12DescriptorHeap> {
    sm::BitMap mAllocator;
    uint mStride{0};

public:
    void create(DeviceHandle &device, DescriptorHeapType type, uint count, bool shader_visible);

    DescriptorIndex acquire();
    void release(DescriptorIndex handle);

    D3D12_CPU_DESCRIPTOR_HANDLE cpu(DescriptorIndex index);
    D3D12_GPU_DESCRIPTOR_HANDLE gpu(DescriptorIndex index);

    ID3D12DescriptorHeap *get() const {
        return Object::get();
    }
};

struct FrameData {
    CommandList commands;

    Object<ID3D12Resource> buffer;
    DescriptorIndex rtv;
    uint64 value = 1;
};

template <typename T>
class ResourcePool {
    sm::BitMap mArena;

protected:
    sm::UniqueArray<T> mStorage;

public:
    constexpr ResourcePool(size_t size)
        : mArena(size)
        , mStorage(size) {}

    size_t length() const {
        return mStorage.length();
    }

    T *acquire() {
        auto index = mArena.scan_set_first();
        CTASSERTF(index != sm::BitMap::eInvalid, "Out of space in bitmap arena");
        return &mStorage[index];
    }

    void release(T *ptr) {
        auto index = ptr - mStorage.get();
        mArena.release(BitMap::Index(index));
    }
};

class CommandListPool : public ResourcePool<CommandList> {
    CommandListType mType;

public:
    constexpr CommandListPool(CommandListType type, size_t size)
        : ResourcePool(size)
        , mType(type) {}

    void create(DeviceHandle &device);

    CommandList *acquire();
};

class PendingObject {
    // std::priority_queue is not very well designed
    // https://stackoverflow.com/questions/20149471/move-out-element-of-std-priority-queue-in-c11
    mutable void *mObject;
    void (*mDispose)(void *, Context &);
    uint64 mValue;

public:
    template <typename T>
    PendingObject(auto &&dispose, T *object, uint64 value)
        : mObject((void *)object)
        , mDispose([](void *object, Context &ctx) {
            using Dispose = decltype(dispose);
            std::invoke(Dispose{}, (T *)object, ctx);
        })
        , mValue(value) {}

    void dispose(Context &ctx) const;

    constexpr auto operator<=>(const PendingObject &other) const {
        return mValue <=> other.mValue;
    }

    constexpr uint64 value() const {
        return mValue;
    }
};

using PendingQueue =
    sm::PriorityQueue<PendingObject, sm::Vector<PendingObject>, std::greater<PendingObject>>;

class Context {
    Sink mSink;
    Instance mInstance;

    static void CALLBACK on_info(D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity,
                                 D3D12_MESSAGE_ID id, LPCSTR desc, void *user);

    size_t mAdapterIndex;
    FeatureLevel mFeatureLevel;
    RootSignatureVersion mRootSignatureVersion;

    Adapter *mAdapter = nullptr;
    DeviceHandle mDevice;
    Object<ID3D12Debug1> mDebug;

    Object<ID3D12InfoQueue1> mInfoQueue;
    DWORD mCookie{UINT_MAX};

    Object<IDXGISwapChain3> mSwapChain;

    size_t mFrameIndex = 0;
    sm::UniqueArray<FrameData> mFrames;

    Object<D3D12MA::Allocator> mAllocator;

    CommandQueue mDirectQueue;
    CommandQueue mCopyQueue;
    CommandQueue mComputeQueue;

    DescriptorArena mRenderTargetHeap;
    DescriptorArena mDepthStencilHeap;
    DescriptorArena mShaderResourceHeap;

    CommandListPool mDirectCommandLists;
    CommandListPool mCopyCommandLists;
    CommandListPool mComputeCommandLists;

    // lists that havent been submitted yet
    sm::Vector<CommandList *> mPendingLists[CommandListType::kCount];

    // objects that have been submitted but not finished execution
    sm::HashMap<Fence *, PendingQueue> mPendingObjects;

    ResourcePool<DeviceResource> mResources;

    Fence mPresentFence;

    Fence mDirectFence;
    Fence mCopyFence;

    void enable_debug_layer(bool gbv, bool rename);
    void enable_dred();
    void enable_info_queue();

    void create_device();
    void query_root_signature_version();
    void create_device_objects(const RenderConfig &config);
    void create_swapchain(const RenderConfig &config);

    void create_allocator();

    void create_backbuffers(const RenderConfig &config);

    void flush_direct_queue();
    void next_frame();

    void reclaim_live_objects(Fence &fence);

    static void reclaim_direct_list(CommandList *list, Context &ctx);
    constexpr static auto kReclaimDirectList = [](CommandList *list, Context &ctx) {
        Context::reclaim_direct_list(list, ctx);
    };

    static void reclaim_copy_list(CommandList *list, Context &ctx);
    constexpr static auto kReclaimCopyList = [](CommandList *list, Context &ctx) {
        Context::reclaim_copy_list(list, ctx);
    };

    static void reclaim_resource(DeviceResource *resource, Context &ctx);
    constexpr static auto kReclaimResource = [](DeviceResource *resource, Context &ctx) {
        Context::reclaim_resource(resource, ctx);
    };

    DeviceResource create_resource(D3D12_HEAP_TYPE heap, D3D12_RESOURCE_DESC desc,
                                   D3D12_RESOURCE_STATES state);

public:
    Context(RenderConfig config);
    ~Context();

    void begin_frame();
    void end_frame();

    DescriptorArena &srv_heap() {
        return mShaderResourceHeap;
    }
    DeviceHandle &device() {
        return mDevice;
    }

    CommandList& acquire_copy_list();
    void submit_copy_list(CommandList &list);

    CommandList &acquire_direct_list();
    void submit_direct_list(CommandList &list);

    DeviceResource create_staging_buffer(uint size);
    DeviceResource create_buffer(uint size);
};
} // namespace sm::render
