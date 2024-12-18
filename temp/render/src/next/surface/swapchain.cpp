#include "stdafx.hpp"

#include "render/next/surface/swapchain.hpp"

using sm::render::next::ISwapChainFactory;
using sm::render::next::ISwapChain;
using sm::render::next::SwapChainLimits;
using sm::render::next::SurfaceInfo;
using sm::render::next::WindowSwapChainFactory;
using sm::render::Object;

#pragma region Utils

static UINT getSwapChainFlags(bool tearing) {
    return tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
}

#pragma region SwapChain

static std::vector<Object<ID3D12Resource>> getSwapChainBuffers(IDXGISwapChain1 *swapchain, UINT length) {
    std::vector<Object<ID3D12Resource>> surfaces{length};
    for (UINT i = 0; i < length; i++) {
        SM_THROW_HR(swapchain->GetBuffer(i, IID_PPV_ARGS(&surfaces[i])));
    }
    return surfaces;
}

class WindowSwapChain final : public ISwapChain {
    Object<IDXGISwapChain3> mSwapChain;
    std::vector<Object<ID3D12Resource>> mBuffers;
    bool mTearingSupport;

    ID3D12Resource *getSurfaceAt(UINT index) override {
        return mBuffers[index].get();
    }

    void updateSurfaces(SurfaceInfo info) override {
        mBuffers.clear();

        auto [width, height] = info.size;
        UINT flags = getSwapChainFlags(mTearingSupport);
        SM_THROW_HR(mSwapChain->ResizeBuffers(info.length, width, height, info.format, flags));

        mBuffers = getSwapChainBuffers(mSwapChain.get(), info.length);
    }

public:
    WindowSwapChain(ISwapChainFactory *factory, Object<IDXGISwapChain3> swapchain, std::vector<Object<ID3D12Resource>> surfaces, bool tearing)
        : ISwapChain(factory, surfaces.size())
        , mSwapChain(std::move(swapchain))
        , mBuffers(std::move(surfaces))
        , mTearingSupport(tearing)
    { }

    UINT currentSurfaceIndex() override {
        return mSwapChain->GetCurrentBackBufferIndex();
    }

    void present(UINT sync) override {
        UINT flags = mTearingSupport ? DXGI_PRESENT_ALLOW_TEARING : 0;
        SM_THROW_HR(mSwapChain->Present(sync, flags));
    }
};

#pragma region Factory

static constexpr SwapChainLimits kSwapChainLimits {
    .minLength = 2,
    .maxLength = DXGI_MAX_SWAP_CHAIN_BUFFERS,
    .minSize = { 16, 16 },
    .maxSize = { 8192 * 2, 8192 * 2 },
};

WindowSwapChainFactory::WindowSwapChainFactory(HWND window)
    : ISwapChainFactory(kSwapChainLimits)
    , mWindow(window)
{ }

ISwapChain *WindowSwapChainFactory::createSwapChain(SurfaceCreateObjects objects, const SurfaceInfo& info) {
    bool tearing = objects.instance.isTearingSupported();
    IDXGIFactory4 *factory = objects.instance.factory();
    ID3D12CommandQueue *queue = objects.queue.get();

    DXGI_SWAP_CHAIN_DESC1 desc {
        .Width = info.size.width,
        .Height = info.size.height,
        .Format = info.format,
        .SampleDesc = { 1, 0 },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = info.length,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .Flags = getSwapChainFlags(tearing),
    };

    Object<IDXGISwapChain1> swapchain;
    SM_THROW_HR(factory->CreateSwapChainForHwnd(queue, mWindow, &desc, nullptr, nullptr, &swapchain));

    if (tearing) {
        SM_THROW_HR(factory->MakeWindowAssociation(mWindow, DXGI_MWA_NO_ALT_ENTER));
    }

    Object<IDXGISwapChain3> swapchain3;
    SM_THROW_HR(swapchain.query(&swapchain3));

    return new WindowSwapChain(this, std::move(swapchain3), getSwapChainBuffers(swapchain.get(), info.length), tearing);
}
