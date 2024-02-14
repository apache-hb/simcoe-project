#pragma once

#include "core/bitmap.hpp"
#include "core/vector.hpp"

#include "logs/sink.hpp"

#include "render/object.hpp"

#include <D3D12MemAlloc.h>

#include "render.reflect.h"

namespace sm::render {
using Sink = logs::Sink<logs::Category::eRender>;

struct DeviceResource {
    Object<ID3D12Resource> resource;
    Object<D3D12MA::Allocation> allocation;
};

struct FrameData {
    Object<ID3D12Resource> backbuffer;
    UINT64 value;
};

struct CommandList {
    Object<ID3D12CommandAllocator> allocator;
    Object<ID3D12GraphicsCommandList> list;
};

struct PipelineState {
    Object<ID3D12PipelineState> state;
    Object<ID3D12RootSignature> signature;
};

struct CommandQueue {
    Object<ID3D12CommandQueue> queue;
    Object<ID3D12Fence> fence;
    HANDLE event;
};

struct DescriptorHandle {
    sm::BitMap::Index index;
    D3D12_CPU_DESCRIPTOR_HANDLE cpu;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu;
};

struct DescriptorArena {
    Object<ID3D12DescriptorHeap> heap;
    sm::BitMap bitmap;
    UINT stride;
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
    logs::ILogger& logger;
};

class Instance {
    Sink mSink;

    Object<IDXGIFactory4> mFactory;
    Object<IDXGIDebug1> mDebug;
    sm::Vector<Object<IDXGIAdapter1>> mAdapters;

    void enum_warp_adapter();
    void enum_adapters();

public:
    Instance(InstanceConfig config);
};

class Context {
    Sink mSink;

    Instance mInstance;

    Object<IDXGISwapChain3> mSwapChain;

    SM_UNUSED size_t mFrameIndex = 0;
    sm::UniqueArray<FrameData> mFrames;

    Object<D3D12MA::Allocator> mAllocator;

    CommandQueue mDirectQueue;
    CommandQueue mCopyQueue;

    DescriptorArena mRenderTargetHeap;
    DescriptorArena mDepthStencilHeap;
    DescriptorArena mShaderResourceHeap;

    ResourcePool<CommandList> mCommandLists;

    ResourcePool<DeviceResource> mResources;
public:
    Context(RenderConfig config);
};
} // namespace sm::render
