#pragma once

#include "core/bitmap.hpp"
#include "core/slotmap.hpp"
#include "core/core.hpp"
#include "core/queue.hpp"
#include "core/span.hpp"
#include "core/vector.hpp"

#include "logs/sink.hpp"

#include "render/object.hpp"

#include <D3D12MemAlloc.h>

#include "render.reflect.h"

namespace sm::render {
constexpr int sm_strcmp(const char *lhs, const char *rhs) {
    while (*lhs && *rhs && *lhs == *rhs) {
        lhs++;
        rhs++;
    }

    return *lhs - *rhs;
}

struct ResourceId {
    const char *name = nullptr;

    constexpr ResourceId() = default;
    constexpr ResourceId(const char *name)
        : name(name)
    { }

    constexpr auto operator<=>(const ResourceId &other) const {
        return sm_strcmp(c_str(), other.c_str()) <=> 0;
    }

    constexpr auto operator==(const ResourceId &other) const {
        return sm_strcmp(c_str(), other.c_str()) == 0;
    }

    constexpr const char *c_str() const { return name != nullptr ? name : "unnamed"; }
};
}
template<>
struct fmt::formatter<sm::render::ResourceId> {
    constexpr auto parse(format_parse_context &ctx) const {
        return ctx.begin();
    }

    auto format(const sm::render::ResourceId &id, format_context &ctx) const {
        return format_to(ctx.out(), "{}", id.c_str());
    }
};

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
    DescriptorRangeFlags flags;
    uint count;
    uint reg;
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
        RootData info;
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
            .type = RootParameterType::eRootConsts,
            .visibility = visibility,
            .info = {reg, space, count},
        };
    }

    static constexpr RootParameter cbv(uint reg, uint space, uint count,
                                       ShaderVisibility visibility) {
        return {
            .type = RootParameterType::eConstBuffer,
            .visibility = visibility,
            .info = {reg, space, count},
        };
    }

    static constexpr RootParameter srv(uint reg, uint space, uint count,
                                       ShaderVisibility visibility) {
        return {
            .type = RootParameterType::eShaderResource,
            .visibility = visibility,
            .info = {reg, space, count},
        };
    }

    static constexpr RootParameter uav(uint reg, uint space, uint count,
                                       ShaderVisibility visibility) {
        return {
            .type = RootParameterType::eUnorderedAccess,
            .visibility = visibility,
            .info = {reg, space, count},
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
    sm::Span<const InputElement> input;
    ShaderBytecode vs;
    ShaderBytecode ps;
};

struct RootSignatureConfig {
    RootSignatureFlags flags;
    sm::Span<const RootParameter> params;
    sm::Span<const Sampler> samplers;
};

struct RootSignature : public Object<ID3D12RootSignature> {
    void create(Context& context, const RootSignatureConfig &config);
};

struct PipelineState : public Object<ID3D12PipelineState> {
    void create(Context& context, RootSignature& signature, const PipelineConfig &config);
};

using Barrier = D3D12_RESOURCE_BARRIER;

Barrier transition_barrier(DeviceResource &resource, ResourceState before,
                        ResourceState after);

struct CommandList {
    Object<ID3D12CommandAllocator> allocator;
    Object<ID3D12GraphicsCommandList> list;

    void create(DeviceHandle &device, CommandListType type);

    void bind_signature(const RootSignature &signature) {
        list->SetGraphicsRootSignature(signature.get());
    }

    void bind_pipeline(const PipelineState &state) {
        list->SetPipelineState(state.get());
    }

    bool is_valid() const {
        return list.is_valid();
    }

    void close();
    void reset();

    void set_index_buffer(const IndexBufferView &view);
    void set_vertex_buffer(const VertexBufferView &view);

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
    uint64 value();
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
    using Alloc = sm::SlotMap<ResourceId>;
    using Index = Alloc::Index;
    using ConstIterator = typename Alloc::ConstIterator;
    Alloc mArena;

protected:
    using Storage = sm::UniqueArray<T>;
    Storage mStorage;

public:
    constexpr ResourcePool(size_t size)
        : mArena(size)
        , mStorage(size)
    { }

    constexpr size_t length() const {
        return mStorage.length();
    }

    T *acquire(ResourceId id) {
        auto index = mArena.alloc(length(), id);

        if (index == Index::eInvalid)
            return nullptr;

        return &mStorage[index];
    }

    void release(T *ptr) {
        auto index = ptr - mStorage.get();
        mArena.release(Index(index));
    }

    constexpr ResourceId get_id(size_t index) const {
        return mArena[index];
    }

    constexpr const T &get_resource(size_t index) {
        return mStorage[index];
    }
};

class CommandListPool : public ResourcePool<CommandList> {
    CommandListType mType;

public:
    constexpr CommandListPool(CommandListType type, size_t size)
        : ResourcePool(size)
        , mType(type) {}

    void create(DeviceHandle &device);

    CommandList *acquire(ResourceId id);
};

template<typename T>
class PendingQueue {
    struct Entry {
        mutable T object;
        uint64 fence;

        constexpr auto operator<=>(const Entry &other) const {
            return fence <=> other.fence;
        }
    };

    sm::PriorityQueue<Entry, std::greater<Entry>> mQueue;

public:
    void dispose(uint64 value, auto&& fn) {
        while (!mQueue.is_empty() && mQueue.top().fence <= value) {
            fn(mQueue.top().object);
            mQueue.pop();
        }
    }

    void push(T object, uint64 fence) {
        mQueue.emplace(std::move(object), fence);
    }

    constexpr uint64 min_value() const {
        return mQueue.is_empty() ? UINT64_MAX : mQueue.top().fence;
    }

    constexpr size_t length() const { return mQueue.length(); }
    constexpr Entry& operator[](size_t index) { return mQueue[index]; }
    constexpr const Entry& operator[](size_t index) const { return mQueue[index]; }
};

using PendingCommandsQueue = PendingQueue<CommandList*>;

class Context {
    friend RootSignature;
    friend PipelineState;

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
    //CommandQueue mComputeQueue;

    DescriptorArena mRenderTargetHeap;
    DescriptorArena mDepthStencilHeap;
    DescriptorArena mShaderResourceHeap;

    CommandListPool mDirectCommandLists;
    CommandListPool mCopyCommandLists;
    //CommandListPool mComputeCommandLists;

    // lists that havent been submitted yet
    sm::Vector<CommandList *> mPendingDirectCommands;
    sm::Vector<CommandList *> mPendingCopyCommands;
    //sm::Vector<CommandList *> mPendingComputeCommands;

    // objects that have been submitted but not finished execution
    //PendingCommandsQueue mWaitingDirectCommands;
    //PendingCommandsQueue mWaitingCopyCommands;
    //PendingCommandsQueue mWaitingComputeCommands;

    //ResourcePool<DeviceResource> mResources;

    Fence mPresentFence;

    //uint64 mDirectFenceValue = 1;
    //Fence mDirectFence;

    uint64 mCopyFenceValue = 1;
    Fence mCopyFence;

    void enable_debug_layer(bool gbv, bool rename);
    void enable_dred();
    void enable_info_queue();

    void create_device();
    void query_root_signature_version();
    void create_device_objects(const RenderConfig &config);
    void create_swapchain(const RenderConfig &config);

    void create_allocator();

    void create_backbuffers(uint count);

    void next_frame();

    //void reclaim_live_objects();

    void reclaim_resource(DeviceResource resource);

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

    void flush_copy_queue();
    void wait_for_gpu();
    //void flush_direct_queue();

    void resize(uint width, uint height);

    CommandList& current_frame_list();

    CommandList& acquire_copy_list(ResourceId id);
    void submit_copy_list(CommandList &list);

    CommandList &acquire_direct_list(ResourceId id);
    void submit_direct_list(CommandList &list);

    DeviceResource create_staging_buffer(uint size);
    DeviceResource create_buffer(uint size);
};
} // namespace sm::render
