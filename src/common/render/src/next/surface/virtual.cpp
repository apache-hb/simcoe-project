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

static VirtualSurface newVirtualSurface(D3D12MA::Allocator *allocator, SurfaceInfo info) {
    return VirtualSurface {
        .target = newSurfaceTexture(allocator, info),
        .readback = newReadbackBuffer(allocator, info),
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
    return mSurfaces[index].getTarget();
}

void VirtualSwapChain::updateSurfaces(SurfaceInfo info) {
    mSurfaces = newSurfaceList(mAllocator, info);
}

VirtualSwapChain::VirtualSwapChain(VirtualSwapChainFactory *factory, SurfaceList surfaces, ID3D12Device1 *device, D3D12MA::Allocator *allocator)
    : ISwapChain(factory, surfaces.size())
    , mSurfaces(std::move(surfaces))
    , mDevice(device)
    , mAllocator(allocator)
{ }

VirtualSwapChain::~VirtualSwapChain() noexcept {
    static_cast<VirtualSwapChainFactory*>(parent())->removeSwapChain(mDevice);
}

UINT VirtualSwapChain::currentSurfaceIndex() {
    return mIndex;
}

void VirtualSwapChain::present(UINT sync) {
    mIndex = (mIndex + 1) % length();
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

    VirtualSwapChain *swapchain = new VirtualSwapChain(this, std::move(surfaces), device, allocator);
    mSwapChains[device] = swapchain;

    return swapchain;
}

void VirtualSwapChainFactory::removeSwapChain(ID3D12Device *device) {
    mSwapChains.erase(device);
}
