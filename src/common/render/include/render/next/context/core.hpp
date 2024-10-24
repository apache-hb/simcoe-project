#pragma once

#include "render/base/instance.hpp"

#include "render/next/render.hpp"
#include "render/next/device.hpp"
#include "render/next/surface/surface.hpp"
#include "render/next/components.hpp"
#include <unordered_set>

namespace sm::render::next {
    class CoreContext;

    struct ContextConfig {
        DebugFlags flags;

        AdapterLUID adapterOverride;
        AdapterPreference autoSearchBehaviour;
        FeatureLevel targetLevel;
        bool allowSoftwareAdapter = false;

        ISwapChainFactory *swapChainFactory = nullptr;
        SurfaceInfo swapChainInfo;

        UINT rtvHeapSize = 0;
        UINT dsvHeapSize = 0;
        UINT srvHeapSize = 0;
    };

    class IContextResource {
    protected:
        CoreContext& mContext;

        IContextResource(CoreContext& context) noexcept
            : mContext(context)
        { }

    public:
        virtual ~IContextResource() noexcept = default;

        virtual void reset() = 0;
        virtual void create() = 0;
        virtual void update(SurfaceInfo info) = 0;
    };

    using ResourceSet = std::unordered_set<std::unique_ptr<IContextResource>>;

    class CoreContext {
        struct DeviceSearchOptions {
            FeatureLevel level;
            DebugFlags flags;
            bool allowSoftwareAdapter;
        };

        struct BackBuffer {
            ID3D12Resource *surface;
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
            uint64_t value;
        };

        using BackBufferList = std::vector<BackBuffer>;

        /// extra limits data
        UINT mExtraRtvHeapSize = 0;
        UINT mExtraDsvHeapSize = 0;
        UINT mExtraSrvHeapSize = 0;

        UINT getRequiredRtvHeapSize() const;
        UINT getRequiredDsvHeapSize() const;
        UINT getRequiredSrvHeapSize() const;

        /// device management

    protected:
        DebugFlags mDebugFlags;
        Instance mInstance;
        CoreDebugState mDebugState;

        CoreDevice mDevice;

        CoreDevice createDevice(AdapterLUID luid, FeatureLevel level, DebugFlags flags);
        RenderResult<CoreDevice> createDevice(Adapter& adapter, FeatureLevel level, DebugFlags flags);

        CoreDevice selectAutoDevice(DeviceSearchOptions options);
        CoreDevice selectPreferredDevice(AdapterLUID luidOverride, DeviceSearchOptions options);

        CoreDevice selectDevice(const ContextConfig& config);

        ResourceSet mDeviceResources;

        template<typename T> requires (std::is_base_of_v<IContextResource, T>)
        T *addResource(auto&&... args) {
            std::unique_ptr<T> resource = std::make_unique<T>(*this, std::forward<decltype(args)>(args)...);
            auto [it, _] = mDeviceResources.emplace(std::move(resource));
            return static_cast<T *>(it->get());
        }

        void removeResource(const IContextResource *resource);

        void resetDeviceResources();
        void createDeviceResources();

        // recreate the current device
        void recreateCurrentDevice();

        // move to a new device
        void moveToNewDevice(AdapterLUID luid);

        /// rtv descriptor heap
        std::unique_ptr<DescriptorPool> mRtvHeap;
        void createRtvHeap();

        /// dsv descriptor heap
        std::unique_ptr<DescriptorPool> mDsvHeap;
        void createDsvHeap();

        /// srv descriptor heap
        std::unique_ptr<DescriptorPool> mSrvHeap;
        void createSrvHeap();

        /// allocator
        Object<D3D12MA::Allocator> mAllocator;
        void createAllocator();

        /// direct queue
        Object<ID3D12CommandQueue> mDirectQueue;
        void createDirectQueue();

        /// direct command list
        std::unique_ptr<CommandBufferSet> mDirectCommandSet;
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
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandleAt(UINT index);
        ID3D12Resource *surfaceAt(UINT index);

        /// device queue fence
        std::unique_ptr<Fence> mPresentFence;
        void createPresentFence();

        /// lifetime management
        void createDeviceState(ISwapChainFactory *swapChainFactory, SurfaceInfo swapChainInfo);

        void beginDeviceSetup();
        void createNewDevice(AdapterLUID luid);

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
        SwapChainLimits getSwapChainLimits() const noexcept { return mSwapChainFactory->limits(); }
        SurfaceInfo getSwapChainInfo() const noexcept { return mSwapChainInfo; }

        /// render object access

        ID3D12Device *getDevice() const noexcept { return mDevice.get(); }
        DescriptorPool& getSrvHeap() noexcept { return *mSrvHeap; }

        /// update rendering device state

        void setAdapter(AdapterLUID luid);
        void updateSwapChain(SurfaceInfo info);

        /// split rendering work into begin/end
        void begin();
        void end();

        /// submit work
        void present();

        /// debugging functions

        bool removeDevice() noexcept;
    };
}
