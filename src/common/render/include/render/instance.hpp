#pragma once

#include <simcoe_config.h>

#include "core/adt/vector.hpp"
#include "core/memory.hpp"

#include "render.reflect.h"

namespace sm::render {
    struct InstanceConfig {
        DebugFlags flags;
        AdapterPreference preference;
    };

    struct DeviceInfo {
        uint vendor;
        uint device;
        uint subsystem;
        uint revision;
    };

    struct AdapterLUID {
        long high;
        uint low;

        AdapterLUID() = default;
        AdapterLUID(LUID luid) : high(luid.HighPart), low(luid.LowPart) { }

        constexpr operator LUID() const { return {low, high}; }

        constexpr bool operator==(const AdapterLUID&) const = default;
    };

    class Adapter : public Object<IDXGIAdapter1> {
        sm::String mName;
        sm::Memory mVideoMemory{0};
        sm::Memory mSystemMemory{0};
        sm::Memory mSharedMemory{0};
        AdapterLUID mAdapterLuid;
        DeviceInfo mDeviceInfo;
        AdapterFlag mFlags;

    public:
        using Object::Object;

        Adapter(IDXGIAdapter1 *adapter);

        constexpr sm::StringView name() const {
            return mName;
        }

        constexpr sm::Memory vidmem() const {
            return mVideoMemory;
        }

        constexpr sm::Memory sysmem() const {
            return mSystemMemory;
        }

        constexpr sm::Memory sharedmem() const {
            return mSharedMemory;
        }

        constexpr AdapterFlag flags() const {
            return mFlags;
        }

        constexpr AdapterLUID luid() const {
            return mAdapterLuid;
        }

        constexpr DeviceInfo info() const {
            return mDeviceInfo;
        }

        constexpr bool operator==(const Adapter& other) const {
            return mAdapterLuid == other.mAdapterLuid;
        }
    };

    class Instance {
        DebugFlags mFlags;
        AdapterPreference mAdapterSearch;

        Object<IDXGIFactory4> mFactory;
        Object<IDXGIDebug1> mDebug;
        sm::Vector<Adapter> mAdapters;
        bool mTearingSupport = false;

        Adapter mWarpAdapter;

#if SMC_WARP_ENABLE
        Library mWarpLibrary;
#endif

        void enableDebugLeakTracking();
        void queryTearingSupport();

        bool enumAdaptersByPreference();
        void findWarpAdapter();
        void enumAdapters();

        void loadWarpRedist();
        void loadPIXRuntime();

    public:
        Instance(InstanceConfig config);
        ~Instance();

        std::span<Adapter> adapters() noexcept;
        Object<IDXGIFactory4> &factory() noexcept;
        const DebugFlags &flags() const noexcept;

        bool isTearingSupported() const noexcept;
        bool isDebugEnabled() const noexcept;
        bool hasViableAdapter() const noexcept;

        Adapter *getAdapterByLUID(LUID luid) noexcept;
        Adapter& getWarpAdapter() noexcept;
        Adapter& getDefaultAdapter() noexcept;
    };
} // namespace sm::render
