#include "stdafx.hpp"

#include "render/next/surface.hpp"

using sm::render::next::ISwapChainFactory;
using sm::render::next::ISwapChain;
using sm::render::next::SwapChainLimits;
using sm::render::Object;

ISwapChainFactory::ISwapChainFactory(SwapChainLimits limits) noexcept
    : mLimits(limits)
{
    CTASSERT(limits.minLength >= 2);
    CTASSERT(limits.maxLength >= limits.minLength);

    CTASSERT(limits.maxSize.x >= limits.minSize.x);
    CTASSERT(limits.maxSize.y >= limits.minSize.y);
}

Object<ID3D12Resource> ISwapChain::getSurface(UINT index) {
    if (index >= mLength)
        throw RenderException{E_INVALIDARG, fmt::format("Invalid swapchain index {}", index)};

    return getSurfaceAt(index);
}

ISwapChain *ISwapChainFactory::newSwapChain(SurfaceCreateObjects objects, const SurfaceInfo& info) {
    if (info.length < mLimits.minLength || info.length > mLimits.maxLength)
        throw RenderException{E_INVALIDARG, fmt::format("Invalid swapchain length: {}", info.length)};

    bool sizeOutOfBounds = info.size.x < mLimits.minSize.x || info.size.x > mLimits.maxSize.x
                       || info.size.y < mLimits.minSize.y || info.size.y > mLimits.maxSize.y;

    if (sizeOutOfBounds) {
        std::string message = fmt::format("Invalid swapchain size: {}x{}", info.size.x, info.size.y);
        throw RenderException{E_INVALIDARG, message};
    }

    return createSwapChain(objects, info);
}
