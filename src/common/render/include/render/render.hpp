#pragma once

#include "core/bitmap.hpp"
#include "core/vector.hpp"

#include "logs/sink.hpp"

#include "render/object.hpp"

#include <D3D12MemAlloc.h>

#include "render.reflect.h"

namespace sm::render {
using Sink = logs::Sink<logs::Category::eRender>;

using DeviceHandle = Object<ID3D12Device1>;

struct DeviceResource {
    Object<ID3D12Resource> resource;
    Object<D3D12MA::Allocation> allocation;
};

struct CommandList {
    Object<ID3D12CommandAllocator> allocator;
    Object<ID3D12GraphicsCommandList> list;

    void create(DeviceHandle& device, CommandListType type);

    void close();
    void reset();

    ID3D12GraphicsCommandList *get() const { return list.get(); }
};

struct PipelineState {
    Object<ID3D12PipelineState> state;
    Object<ID3D12RootSignature> signature;
};

constexpr auto kCloseHandle = [](HANDLE handle) { CloseHandle(handle); };

struct Fence {
    Object<ID3D12Fence> fence;
    sm::UniqueHandle<HANDLE, decltype(kCloseHandle)> event;

    void create(DeviceHandle& device, const char *name = nullptr);

    void wait(UINT64 value);
};

struct CommandQueue {
    Object<ID3D12CommandQueue> queue;

    void create(DeviceHandle& device, CommandListType type);
    void signal(Fence& fence, UINT64 value);
    void execute(UINT count, ID3D12CommandList *const *lists);

    ID3D12CommandQueue *get() const { return queue.get(); }
};

enum DescriptorIndex : UINT { eInvalid = UINT_MAX };

class DescriptorArena {
    Object<ID3D12DescriptorHeap> mDescriptorHeap;
    sm::BitMap mAllocator;
    UINT mStride{0};

public:
    void create(DeviceHandle& device, DescriptorHeapType type, UINT count, bool shader_visible);

    DescriptorIndex acquire();
    void release(DescriptorIndex handle);

    D3D12_CPU_DESCRIPTOR_HANDLE cpu(DescriptorIndex index);
    D3D12_GPU_DESCRIPTOR_HANDLE gpu(DescriptorIndex index);
};

struct FrameData {
    CommandList commands;

    Object<ID3D12Resource> buffer;
    DescriptorIndex rtv;
    UINT64 value = 1;
};

template<typename T>
class ResourcePool {
    sm::BitMap mArena;
    sm::UniquePtr<T[]> mStorage;

public:
    ResourcePool(size_t size)
        : mArena(size)
    { }

    T *acquire() {
        auto index = mArena.scan_set_first();
        CTASSERTF(index != sm::BitMap::eInvalid, "Out of space in bitmap arena");
        return &mStorage[index];
    }

    void release(T *ptr) {
        auto index = ptr - mStorage.get();
        mArena.release(index);
    }
};

struct InstanceConfig {
    DebugFlags flags;
    AdapterPreference preference;
    logs::ILogger& logger;
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

    constexpr std::string_view name() const { return mName; }

    constexpr sm::Memory vidmem() const { return mVideoMemory; }
    constexpr sm::Memory sysmem() const { return mSystemMemory; }
    constexpr sm::Memory sharedmem() const { return mSharedMemory; }

    constexpr AdapterFlag flags() const { return mFlags; }
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

    Adapter &warp_adapter() { return mWarpAdapter; }
    Adapter &get_adapter(size_t index) { return mAdapters[index]; }

    Object<IDXGIFactory4> &factory() { return mFactory; }

    const DebugFlags &flags() const { return mFlags; }
};

class Context {
    Sink mSink;
    Instance mInstance;

    static void CALLBACK on_info(
        D3D12_MESSAGE_CATEGORY category,
        D3D12_MESSAGE_SEVERITY severity,
        D3D12_MESSAGE_ID id,
        LPCSTR desc,
        void *user);

    size_t mAdapterIndex;
    FeatureLevel mFeatureLevel;
    Adapter *mAdapter = nullptr;
    DeviceHandle mDevice;

    Object<ID3D12Debug1> mDebug;

    Object<ID3D12InfoQueue1> mInfoQueue;
    DWORD mCookie{UINT_MAX};

    Object<IDXGISwapChain3> mSwapChain;

    SM_UNUSED size_t mFrameIndex = 0;
    sm::UniqueArray<FrameData> mFrames;

    Object<D3D12MA::Allocator> mAllocator;

    CommandQueue mDirectQueue;
    CommandQueue mCopyQueue;
    CommandQueue mComputeQueue;

    DescriptorArena mRenderTargetHeap;
    DescriptorArena mDepthStencilHeap;
    DescriptorArena mShaderResourceHeap;

    ResourcePool<CommandList> mDirectCommandLists;
    ResourcePool<CommandList> mCopyCommandLists;
    ResourcePool<CommandList> mComputeCommandLists;

    ResourcePool<DeviceResource> mResources;

    Fence mPresentFence;

    void enable_debug_layer(bool gbv, bool rename);
    void enable_dred();
    void enable_info_queue();

    void create_device();
    void create_device_objects(const RenderConfig& config);
    void create_swapchain(const RenderConfig& config);

    void create_allocator();

    void create_backbuffers(const RenderConfig& config);

    void flush_direct_queue();
    void next_frame();

public:
    Context(RenderConfig config);
    ~Context();

    void begin_frame();
    void end_frame();
};
} // namespace sm::render
