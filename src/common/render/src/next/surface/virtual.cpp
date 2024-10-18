#include "stdafx.hpp"

#include "render/next/surface.hpp"

namespace math = sm::math;

using sm::render::next::ISwapChainFactory;
using sm::render::next::ISwapChain;
using sm::render::next::SwapChainLimits;
using sm::render::next::SurfaceInfo;
using sm::render::next::VirtualSwapChainFactory;
using sm::render::Object;

using SurfaceList = std::vector<Object<ID3D12Resource>>;

struct VirtualSurface {
    Object<ID3D12Resource> target;
    Object<ID3D12Resource> readback;
};

static const D3D12_HEAP_PROPERTIES kDefaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
static const D3D12_HEAP_PROPERTIES kReadbackHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);

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

static SurfaceList newSurfaceList(ID3D12Device1 *device, SurfaceInfo info) {
    SurfaceList surfaces{info.length};

    for (UINT i = 0; i < info.length; i++) {
        surfaces[i] = newSurfaceTexture(device, info);
    }

    return surfaces;
}

#pragma region SwapChain

class VirtualSwapChain final : public ISwapChain {
    UINT mIndex = 0;
    SurfaceList mSurfaces;
    ID3D12Device1 *mDevice;

    Object<ID3D12Resource> getSurfaceAt(UINT index) override {
        return mSurfaces[index].clone();
    }

    void updateSurfaces(SurfaceInfo info) override {
        mSurfaces = newSurfaceList(mDevice, info);
    }

public:
    VirtualSwapChain(ISwapChainFactory *factory, SurfaceList surfaces, ID3D12Device1 *device)
        : ISwapChain(factory, surfaces.size())
        , mSurfaces(std::move(surfaces))
        , mDevice(device)
    { }

    UINT currentSurfaceIndex() override {
        return mIndex;
    }

    void present(UINT sync) override {
        mIndex = (mIndex + 1) % length();
    }
};

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

    return new VirtualSwapChain(this, std::move(surfaces), device);
}
