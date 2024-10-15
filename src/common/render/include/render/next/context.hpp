#pragma once

#include "render/next/render.hpp"

#include "render/base/instance.hpp"

namespace sm::render::next {
    struct ContextConfig {
        DebugFlags flags;

        AdapterLUID adapterOverride;
        AdapterPreference autoSearchBehaviour;
        FeatureLevel targetLevel;
        bool allowSoftwareAdapter = false;
    };

    class CoreDebugState {
        Object<ID3D12Debug1> mDebug;

        bool mDebugLayer = false;
        bool mGpuValidation = false;
        bool mAutoNamedObjects = false;
        bool mDeviceRemovedInfo = false;

        bool setupDebugLayer(bool enabled);

    public:
        CoreDebugState(DebugFlags flags) throws(RenderException);
    };

    class CoreDevice {
        FeatureLevel mFeatureLevel;
        AdapterLUID mAdapterLUID;
        Object<ID3D12Device1> mDevice;

        Object<ID3D12InfoQueue1> mInfoQueue;
        DWORD mCookie = ULONG_MAX;

        void setupCoreDevice(Adapter& adapter, FeatureLevel level);
        void setupInfoQueue(bool enabled);

    public:
        CoreDevice(Adapter& adapter, FeatureLevel level, DebugFlags flags) throws(RenderException);

        FeatureLevel level() const { return mFeatureLevel; }
    };

    class CoreContext {
        struct DeviceSearchOptions {
            FeatureLevel level;
            DebugFlags flags;
            bool allowSoftwareAdapter;
        };

        Instance mInstance;
        CoreDebugState mDebugState;

        CoreDevice mDevice;

        RenderResult<CoreDevice> createDevice(Adapter& adapter, FeatureLevel level, DebugFlags flags);

        CoreDevice selectAutoDevice(DeviceSearchOptions options);
        CoreDevice selectPreferredDevice(AdapterLUID luidOverride, DeviceSearchOptions options);

        CoreDevice selectDevice(const ContextConfig& config);

    public:
        CoreContext(ContextConfig config) throws(RenderException);
    };
}
