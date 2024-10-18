#pragma once

#include "render/base/instance.hpp"

#include "render/next/render.hpp"
#include "render/next/device.hpp"

namespace sm::render::next {
    struct DeviceContextConfig {
        DebugFlags flags;

        AdapterLUID adapterOverride;
        AdapterPreference autoSearchBehaviour;
        FeatureLevel targetLevel;
        bool allowSoftwareAdapter = false;
    };

    class IDeviceBound {
    public:
        virtual ~IDeviceBound() noexcept = default;

        virtual void resetDeviceResources() = 0;
    };

    class DeviceContext {
        struct DeviceSearchOptions {
            FeatureLevel level;
            DebugFlags flags;
            bool allowSoftwareAdapter;
        };

        /// device management

        DebugFlags mDebugFlags;
        Instance mInstance;
        CoreDebugState mDebugState;

        CoreDevice mDevice;

        CoreDevice createDevice(AdapterLUID luid, FeatureLevel level, DebugFlags flags);
        RenderResult<CoreDevice> createDevice(Adapter& adapter, FeatureLevel level, DebugFlags flags);

        CoreDevice selectAutoDevice(DeviceSearchOptions options);
        CoreDevice selectPreferredDevice(AdapterLUID luidOverride, DeviceSearchOptions options);

        CoreDevice selectDevice(const DeviceContextConfig& config);

        void resetDeviceResources();

        void recreateCurrentDevice();

        void moveToNewDevice(AdapterLUID luid);

        /// allocator management
        Object<D3D12MA::Allocator> mAllocator;

        std::set<IDeviceBound*> mDeviceBoundResources;
    public:
        DeviceContext(DeviceContextConfig config) throws(RenderException);

        Instance& instance() noexcept { return mInstance; }
        CoreDevice& device() noexcept { return mDevice; }
        D3D12MA::Allocator *allocator() noexcept { return mAllocator.get(); }

        void setAdapter(AdapterLUID luid);

        void bindResource(IDeviceBound *resource) { mDeviceBoundResources.insert(resource); }
        void unbindResource(IDeviceBound *resource) { mDeviceBoundResources.erase(resource); }

        void setDeviceRemoved();
    };
}
