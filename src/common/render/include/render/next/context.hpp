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

        UINT rtvHeapSize = 0;
    };

    class CoreContext {
        struct DeviceSearchOptions {
            FeatureLevel level;
            DebugFlags flags;
            bool allowSoftwareAdapter;
        };

        struct BackBuffer {
            Object<ID3D12Resource> surface;
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
            uint64_t value;
        };

        using BackBufferList = std::vector<BackBuffer>;

        /// extra limits data
        UINT mExtraRtvHeapSize = 0;

        UINT getRequiredRtvHeapSize() const;

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

        void resetDeviceResources();

        // recreate the current device
        void recreateCurrentDevice();

        // move to a new device
        void moveToNewDevice(AdapterLUID luid);

        /// rtv descriptor heap
        std::unique_ptr<DescriptorPool> mRtvHeap;
        void createRtvHeap();

        /// allocator
        Object<D3D12MA::Allocator> mAllocator;
        void createAllocator();

        /// direct queue
        Object<ID3D12CommandQueue> mDirectQueue;
        void createDirectQueue();

        /// direct command list
        std::unique_ptr<CommandBufferSet> mCommandBufferSet;
        void createDirectCommandList();

        /// swapchain
        ISwapChainFactory *mSwapChainFactory;
        std::unique_ptr<ISwapChain> mSwapChain;
        SurfaceInfo mSwapChainInfo;
        void createSwapChain(ISwapChainFactory *factory, SurfaceInfo info);

        /// swapchain backbuffers
        BackBufferList mBackBuffers;
        UINT mCurrentBackBuffer = 0;
        void createBackBuffers(uint64_t initialValue);
        BackBufferList getSwapChainSurfaces(uint64_t initialValue) const;
        uint64_t& fenceValueAt(UINT index);

        /// device queue fence
        std::unique_ptr<Fence> mPresentFence;
        void createPresentFence();

        /// lifetime management
        void createDeviceState();

        /// gpu timeline
        void advanceFrame();
        void flushDevice();
        void setFrameIndex(UINT index);

    public:
        CoreContext(ContextConfig config) throws(RenderException);
        ~CoreContext() noexcept;

        /// state queries

        std::span<const Adapter> adapters() const noexcept { return mInstance.adapters(); }
        AdapterLUID getAdapter() const noexcept { return mDevice.luid(); }
        const Adapter& getWarpAdapter() noexcept { return mInstance.getWarpAdapter(); }

        /// update rendering device state

        void setAdapter(AdapterLUID luid);
        void updateSwapChain(SurfaceInfo info);

        /// submit work

        void present();

        /// debugging functions

        bool removeDevice() noexcept;
    };
}
