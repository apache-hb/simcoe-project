#pragma once

#include "render/base/instance.hpp"

#include "render/next/render.hpp"
#include "render/next/device.hpp"
#include "render/next/surface.hpp"
#include "render/next/components.hpp"

namespace sm::render::next {
    struct ContextConfig {
        DebugFlags flags;

        AdapterLUID adapterOverride;
        AdapterPreference autoSearchBehaviour;
        FeatureLevel targetLevel;
        bool allowSoftwareAdapter = false;

        ISwapChainFactory *swapChainFactory = nullptr;
        SurfaceInfo swapChainInfo;
    };

    class CoreContext {
        struct DeviceSearchOptions {
            FeatureLevel level;
            DebugFlags flags;
            bool allowSoftwareAdapter;
        };

        using SurfaceList = std::vector<Object<ID3D12Resource>>;

        /// device management

        DebugFlags mDebugFlags;
        Instance mInstance;
        CoreDebugState mDebugState;

        CoreDevice mDevice;

        CoreDevice createDevice(AdapterLUID luid, FeatureLevel level, DebugFlags flags);
        RenderResult<CoreDevice> createDevice(Adapter& adapter, FeatureLevel level, DebugFlags flags);

        CoreDevice selectAutoDevice(DeviceSearchOptions options);
        CoreDevice selectPreferredDevice(AdapterLUID luidOverride, DeviceSearchOptions options);

        CoreDevice selectDevice(const ContextConfig& config);

        /// allocator
        Object<D3D12MA::Allocator> mAllocator;
        void createAllocator();

        /// direct queue
        Object<ID3D12CommandQueue> mDirectQueue;
        void createDirectQueue();

        /// swapchain
        ISwapChainFactory *mSwapChainFactory;
        std::unique_ptr<ISwapChain> mSwapChain;
        SurfaceInfo mSwapChainInfo;
        void createSwapChain(ISwapChainFactory *factory, SurfaceInfo info);

        /// swapchain backbuffers
        SurfaceList mBackBuffers;
        UINT mCurrentBackBuffer = 0;
        void createBackBuffers(UINT initialValue);
        SurfaceList getSwapChainSurfaces() const;

        /// device queue fence
        std::unique_ptr<Fence> mPresentFence;
        std::vector<uint64_t> mFenceValues;
        void createPresentFence();

        /// lifetime management
        void createDeviceState();

        /// gpu timeline
        void advanceFrame();
        void flushDevice();
        void setFrameIndex(UINT index);

    public:
        CoreContext(ContextConfig config) throws(RenderException);

        std::span<const Adapter> adapters() const noexcept { return mInstance.adapters(); }
        const Adapter& getWarpAdapter() noexcept { return mInstance.getWarpAdapter(); }
        void setAdapter(AdapterLUID luid);

        void updateSwapChain(SurfaceInfo info);

        void present();
    };
}
