#include "stdafx.hpp"

#include "render/next/surface/virtual.hpp"

namespace math = sm::math;

using namespace sm::render::next;

using sm::render::Object;

static const D3D12_HEAP_PROPERTIES kDefaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
static const D3D12_HEAP_PROPERTIES kReadHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);

#pragma region Utils

static D3D12MA::Allocation *newComittedResource(D3D12MA::Allocator *allocator, D3D12_HEAP_TYPE heap, const D3D12_RESOURCE_DESC *desc, D3D12_RESOURCE_STATES state, const D3D12_CLEAR_VALUE *clear) {
    const D3D12MA::ALLOCATION_DESC cAllocInfo {
        .Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED,
        .HeapType = heap,
    };

    D3D12MA::Allocation *allocation = nullptr;
    SM_THROW_HR(allocator->CreateResource(&cAllocInfo, desc, state, clear, &allocation, __uuidof(ID3D12Resource), nullptr));
    return allocation;
}

static D3D12MA::Allocation *newSurfaceTexture(D3D12MA::Allocator *allocator, SurfaceInfo info) {
    auto [format, size, _, clear] = info;
    const D3D12_RESOURCE_DESC cTextureInfo = CD3DX12_RESOURCE_DESC::Tex2D(
        format, size.width, size.height,
        1, 0, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
    );

    const D3D12_CLEAR_VALUE cClearValue = CD3DX12_CLEAR_VALUE(format, clear.data());

    return newComittedResource(allocator, D3D12_HEAP_TYPE_DEFAULT, &cTextureInfo, D3D12_RESOURCE_STATE_PRESENT, &cClearValue);
}

static D3D12MA::Allocation *newReadbackBuffer(D3D12MA::Allocator *allocator, SurfaceInfo info) {
    auto [format, size, _, _] = info;

    UINT64 pitch = size.width * 4LLU;
    UINT64 bytes = pitch * size.height;
    const D3D12_RESOURCE_DESC cBufferInfo = CD3DX12_RESOURCE_DESC::Buffer(bytes);

    return newComittedResource(allocator, D3D12_HEAP_TYPE_READBACK, &cBufferInfo, D3D12_RESOURCE_STATE_COPY_DEST, nullptr);
}

static std::span<const uint32_t> mapBuffer(D3D12MA::Allocation *readback, SurfaceInfo info) {
    auto [format, size, _, _] = info;
    uint64_t bytes = size.width * size.height * 4ULL; // NOLINT
    ID3D12Resource *resource = readback->GetResource();

    const D3D12_RANGE cReadRange = { 0, bytes };
    void *data = nullptr;
    SM_THROW_HR(resource->Map(0, &cReadRange, &data));

    return std::span<const uint32_t>(static_cast<const uint32_t*>(data), bytes);
}

static VirtualSurface newVirtualSurface(D3D12MA::Allocator *allocator, SurfaceInfo info) {
    D3D12MA::Allocation *target = newSurfaceTexture(allocator, info);
    D3D12MA::Allocation *readback = newReadbackBuffer(allocator, info);
    std::span<const uint32_t> image = mapBuffer(readback, info);

    return VirtualSurface {
        .target = target,
        .readback = readback,
        .image = image
    };
}

static SurfaceList newSurfaceList(D3D12MA::Allocator *allocator, SurfaceInfo info) {
    SurfaceList surfaces{info.length};

    for (UINT i = 0; i < info.length; i++) {
        surfaces[i] = newVirtualSurface(allocator, info);
    }

    return surfaces;
}

#pragma region SwapChain

ID3D12Resource *VirtualSwapChain::getSurfaceAt(UINT index) {
    return mSurfaces[index].getRenderTarget();
}

void VirtualSwapChain::updateSurfaces(SurfaceInfo info) {
    mCommandList = CommandBufferSet(mDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, info.length);
    mSurfaces = newSurfaceList(mAllocator, info);
    mSurfaceInfo = info;
    mIndex = 0;
}

VirtualSwapChain::VirtualSwapChain(VirtualSwapChainFactory *factory, SurfaceInfo info, SurfaceList surfaces, SurfaceCreateObjects objects)
    : ISwapChain(factory, surfaces.size())
    , mSurfaces(std::move(surfaces))
    , mSurfaceInfo(info)
    , mDevice(objects.device)
    , mAllocator(objects.allocator.get())
    , mCommandQueue(objects.queue.get())
    , mCommandList(objects.device, D3D12_COMMAND_LIST_TYPE_DIRECT, mSurfaces.size())
{ }

VirtualSwapChain::~VirtualSwapChain() noexcept {
    static_cast<VirtualSwapChainFactory*>(parent())->removeSwapChain(mDevice.get());
}

UINT VirtualSwapChain::currentSurfaceIndex() {
    return mIndex;
}

void VirtualSwapChain::present(UINT sync) {
    /// gather all required resources to do readback copy
    ID3D12Resource *source = getSurfaceAt(mIndex);
    ID3D12Resource *readback = getCopyTarget(mIndex);

    // we expect the source to be in the present state
    const D3D12_RESOURCE_BARRIER cIntoCopySource[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(source, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_SOURCE),
    };

    const D3D12_RESOURCE_BARRIER cIntoPresent[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(source, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PRESENT),
    };

    const D3D12_PLACED_SUBRESOURCE_FOOTPRINT cBufferInfo {
        .Footprint = {
            .Format = mSurfaceInfo.format,
            .Width = mSurfaceInfo.size.width,
            .Height = mSurfaceInfo.size.height,
            .Depth = 1,
            .RowPitch = mSurfaceInfo.size.width * 4,
        },
    };

    const CD3DX12_TEXTURE_COPY_LOCATION cCopySource { source, 0 };
    const CD3DX12_TEXTURE_COPY_LOCATION cCopyDest { readback, cBufferInfo };

    // prepare the commands

    mCommandList->ResourceBarrier(_countof(cIntoCopySource), cIntoCopySource);

    mCommandList->CopyTextureRegion(&cCopyDest, 0, 0, 0, &cCopySource, nullptr);

    mCommandList->ResourceBarrier(_countof(cIntoPresent), cIntoPresent);

    // submit

    ID3D12CommandList *lists[] = { mCommandList.close() };
    mCommandQueue->ExecuteCommandLists(_countof(lists), lists);

    // prepare for the next frame

    mIndex = (mIndex + 1) % length();

    mCommandList.reset(mIndex);
}

#pragma region Factory

static constexpr SwapChainLimits kSwapChainLimits {
    .minLength = 2,
    .maxLength = 16,
    .minSize = { 16, 16 },
    .maxSize = { D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION, D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION  },
};

VirtualSwapChainFactory::VirtualSwapChainFactory()
    : ISwapChainFactory(kSwapChainLimits)
{ }

ISwapChain *VirtualSwapChainFactory::createSwapChain(SurfaceCreateObjects objects, const SurfaceInfo& info) {
    ID3D12Device1 *device = objects.device.get();
    D3D12MA::Allocator *allocator = objects.allocator.get();

    SurfaceList surfaces = newSurfaceList(allocator, info);

    VirtualSwapChain *swapchain = new VirtualSwapChain(this, info, std::move(surfaces), objects);
    mSwapChains[device] = swapchain;

    return swapchain;
}

void VirtualSwapChainFactory::removeSwapChain(ID3D12Device *device) {
    mSwapChains.erase(device);
}
