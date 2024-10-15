#include "stdafx.hpp"

#include "render/next/surface.hpp"

using sm::render::next::VirtualSwapChainFactory;
using sm::render::next::ISwapChain;
using sm::render::next::SwapChainLimits;
using sm::render::next::SurfaceInfo;
using sm::render::Object;

using SurfaceList = std::vector<Object<ID3D12Resource>>;

#pragma region SwapChain

class VirtualSwapChain final : public ISwapChain {
    UINT mIndex = 0;
    SurfaceList mSurfaces;

    Object<ID3D12Resource> getSurfaceAt(UINT index) override {
        return mSurfaces[index].clone();
    }

public:
    VirtualSwapChain(SurfaceList surfaces)
        : ISwapChain(surfaces.size())
        , mSurfaces(std::move(surfaces))
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
    .maxLength = 64,
    .minSize = { 16, 16 },
    .maxSize = { D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION, D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION  },
};

static const D3D12_HEAP_PROPERTIES kDefaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

VirtualSwapChainFactory::VirtualSwapChainFactory()
    : ISwapChainFactory(kSwapChainLimits)
{ }

ISwapChain *VirtualSwapChainFactory::createSwapChain(SurfaceCreateObjects objects, const SurfaceInfo& info) {
    const D3D12_RESOURCE_DESC kTextureInfo = CD3DX12_RESOURCE_DESC::Tex2D(
        info.format, info.size.width, info.size.height,
        1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
    );

    const D3D12_CLEAR_VALUE kClearValue = CD3DX12_CLEAR_VALUE(info.format, info.clearColour.data());

    ID3D12Device *device = objects.device.get();

    SurfaceList surfaces{info.length};

    for (UINT i = 0; i < info.length; i++) {
        SM_THROW_HR(device->CreateCommittedResource(&kDefaultHeap, D3D12_HEAP_FLAG_NONE, &kTextureInfo, D3D12_RESOURCE_STATE_PRESENT, &kClearValue, IID_PPV_ARGS(&surfaces[i])));
    }

    return new VirtualSwapChain(std::move(surfaces));
}
