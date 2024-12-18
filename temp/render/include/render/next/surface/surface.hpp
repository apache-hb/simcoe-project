#pragma once

#include "render/next/device.hpp"

namespace sm::render::next {
    class ISwapChain;
    class ISwapChainFactory;

    struct SurfaceCreateObjects {
        Instance& instance;
        CoreDevice& device;
        Object<D3D12MA::Allocator>& allocator;
        Object<ID3D12CommandQueue>& queue;
    };

    struct SurfaceInfo {
        DXGI_FORMAT format;
        math::uint2 size;
        UINT length;
        math::float4 clear;
    };

    struct SwapChainLimits {
        UINT minLength;
        UINT maxLength;

        math::uint2 minSize;
        math::uint2 maxSize;
    };

    class ISwapChain {
        ISwapChainFactory *mFactory;
        UINT mLength;

        virtual ID3D12Resource *getSurfaceAt(UINT index) = 0;

        virtual void updateSurfaces(SurfaceInfo info) = 0;

    protected:
        ISwapChain(ISwapChainFactory *factory, UINT length) noexcept
            : mFactory(factory)
            , mLength(length)
        { }

        ISwapChainFactory *parent() const noexcept { return mFactory; }

    public:
        virtual ~ISwapChain() = default;

        ID3D12Resource *getSurface(UINT index);
        UINT length() const noexcept { return mLength; }

        void updateSurfaceInfo(SurfaceInfo info);

        virtual UINT currentSurfaceIndex() = 0;
        virtual void present(UINT sync) = 0;
    };

    class ISwapChainFactory {
        SwapChainLimits mLimits;

        virtual ISwapChain *createSwapChain(SurfaceCreateObjects objects, const SurfaceInfo& info) = 0;

    protected:
        ISwapChainFactory(SwapChainLimits limits) noexcept;

    public:
        virtual ~ISwapChainFactory() = default;

        ISwapChain *newSwapChain(SurfaceCreateObjects objects, const SurfaceInfo& info);

        SwapChainLimits limits() const noexcept { return mLimits; }
        UINT maxSwapChainLength() const noexcept { return mLimits.maxLength; }
    };
}
