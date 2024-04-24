#pragma once

#include <simcoe_config.h>

#include "core/vector.hpp"
#include "core/memory.hpp"
#include "core/library.hpp"

#include "render.reflect.h"

namespace sm {
    LOG_CATEGORY(gGpuLog);
}

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

        void enable_leak_tracking();
        void query_tearing_support();

        bool enum_by_preference();
        void enum_warp_adapter();
        void enum_adapters();

        void load_warp_redist();
        void load_pix_runtime();

    public:
        Instance(InstanceConfig config);
        ~Instance();

        sm::Vector<Adapter> &get_adapters();
        Object<IDXGIFactory4> &factory();
        const DebugFlags &flags() const;
        bool tearing_support() const;
        bool debug_support() const;
        bool has_viable_adapter() const;

        Adapter *get_adapter_by_luid(LUID luid);
        Adapter& get_warp_adapter();
        Adapter& get_default_adapter();
    };
} // namespace sm::render
