#include "stdafx.hpp"

#include "render/next/surface/virtual.hpp"

namespace math = sm::math;

using namespace sm::render::next;

using sm::render::Object;

static const D3D12_HEAP_PROPERTIES kDefaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
static const D3D12_HEAP_PROPERTIES kReadHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);

#pragma region Utils

static ID3D12Resource *newSurfaceTexture(ID3D12Device1 *device, SurfaceInfo info) {
    auto [format, size, _, clear] = info;
    const D3D12_RESOURCE_DESC kTextureInfo = CD3DX12_RESOURCE_DESC::Tex2D(
        format, size.width, size.height,
        1, 0, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
    );

    const D3D12_CLEAR_VALUE kClearValue = CD3DX12_CLEAR_VALUE(format, clear.data());

    ID3D12Resource *resource = nullptr;
    SM_THROW_HR(device->CreateCommittedResource(&kDefaultHeap, D3D12_HEAP_FLAG_NONE, &kTextureInfo, D3D12_RESOURCE_STATE_PRESENT, &kClearValue, IID_PPV_ARGS(&resource)));
    return resource;
}

static ID3D12Resource *newReadbackBuffer(ID3D12Device1 *device, SurfaceInfo info) {
    auto [format, size, _, _] = info;

    UINT64 pitch = size.width * 4LLU;
    UINT64 bytes = pitch * size.height;
    const D3D12_RESOURCE_DESC kBufferInfo = CD3DX12_RESOURCE_DESC::Buffer(bytes);

    ID3D12Resource *resource = nullptr;
    SM_THROW_HR(device->CreateCommittedResource(&kReadHeap, D3D12_HEAP_FLAG_NONE, &kBufferInfo, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resource)));
    return resource;
}

static VirtualSurface newVirtualSurface(ID3D12Device1 *device, SurfaceInfo info) {
    return VirtualSurface {
        .target = newSurfaceTexture(device, info),
        .readback = newReadbackBuffer(device, info),
    };
}

static SurfaceList newSurfaceList(ID3D12Device1 *device, SurfaceInfo info) {
    SurfaceList surfaces{info.length};

    for (UINT i = 0; i < info.length; i++) {
        surfaces[i] = newVirtualSurface(device, info);
    }

    return surfaces;
}

#pragma region SwapChain

Object<ID3D12Resource> VirtualSwapChain::getSurfaceAt(UINT index) {
    return mSurfaces[index].getTarget();
}

void VirtualSwapChain::updateSurfaces(SurfaceInfo info) {
    mSurfaces = newSurfaceList(mDevice, info);
}

VirtualSwapChain::VirtualSwapChain(VirtualSwapChainFactory *factory, SurfaceList surfaces, ID3D12Device1 *device)
    : ISwapChain(factory, surfaces.size())
    , mSurfaces(std::move(surfaces))
    , mDevice(device)
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

    SurfaceList surfaces = newSurfaceList(device, info);

    VirtualSwapChain *swapchain = new VirtualSwapChain(this, std::move(surfaces), device);
    mSwapChains[device] = swapchain;

    return swapchain;
}

void VirtualSwapChainFactory::removeSwapChain(ID3D12Device *device) {
    mSwapChains.erase(device);
}
