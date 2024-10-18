#include "stdafx.hpp"

#include "render/next/surface/surface.hpp"

using sm::render::next::ISwapChainFactory;
using sm::render::next::ISwapChain;
using sm::render::next::SwapChainLimits;
using sm::render::next::SurfaceInfo;
using sm::render::Object;
using sm::render::RenderException;

static void checkSurfaceLimits(const SwapChainLimits& limits, const SurfaceInfo& info) {
    if (info.length < limits.minLength || info.length > limits.maxLength)
        throw RenderException{E_INVALIDARG, fmt::format("Invalid swapchain length: {}", info.length)};

    bool sizeOutOfBounds = info.size.x < limits.minSize.x || info.size.x > limits.maxSize.x
                       || info.size.y < limits.minSize.y || info.size.y > limits.maxSize.y;

    if (sizeOutOfBounds) {
        std::string message = fmt::format("Invalid swapchain size: {}x{}", info.size.x, info.size.y);
        throw RenderException{E_INVALIDARG, message};
    }
}

ISwapChainFactory::ISwapChainFactory(SwapChainLimits limits) noexcept
    : mLimits(limits)
{
    CTASSERT(limits.minLength >= 2);
    CTASSERT(limits.maxLength >= limits.minLength);

    CTASSERT(limits.maxSize.x >= limits.minSize.x);
    CTASSERT(limits.maxSize.y >= limits.minSize.y);
}

ISwapChain *ISwapChainFactory::newSwapChain(SurfaceCreateObjects objects, const SurfaceInfo& info) {
    checkSurfaceLimits(limits(), info);

    return createSwapChain(objects, info);
}

Object<ID3D12Resource> ISwapChain::getSurface(UINT index) {
    if (index >= mLength)
        throw RenderException{E_INVALIDARG, fmt::format("Invalid swapchain index {}", index)};

    return getSurfaceAt(index);
}

void ISwapChain::updateSurfaceInfo(SurfaceInfo info) {
    SwapChainLimits limits = mFactory->limits();
    checkSurfaceLimits(limits, info);

    updateSurfaces(info);
    mLength = info.length;
}
