#pragma once

#include <simcoe_config.h>

#include "core/memory.hpp"

#include "render/base/object.hpp"

#include "render.reflect.h"

namespace sm::db {
    class Connection;
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

        constexpr auto operator<=>(const AdapterLUID&) const = default;
    };

    class Adapter : public Object<IDXGIAdapter1> {
        std::string mName;
        sm::Memory mVideoMemory{0};
        sm::Memory mSystemMemory{0};
        sm::Memory mSharedMemory{0};
        AdapterLUID mAdapterLuid;
        DeviceInfo mDeviceInfo;
        AdapterFlag mFlags;

    public:
        using Object::Object;

        Adapter(IDXGIAdapter1 *adapter);

        constexpr std::string_view name() const {
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
        bool mTearingSupport = false;
        DebugFlags mFlags;
        AdapterPreference mAdapterSearch;

        Object<IDXGIFactory4> mFactory;
        Object<IDXGIDebug1> mDebug;
        std::vector<Adapter> mAdapters;

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
        ~Instance() noexcept;

        std::span<Adapter> adapters() noexcept { return mAdapters; }
        std::span<const Adapter> adapters() const noexcept { return mAdapters; }
        Object<IDXGIFactory4> &factory() noexcept { return mFactory; }
        const DebugFlags &flags() const noexcept { return mFlags; }
        bool isTearingSupported() const noexcept { return mTearingSupport; }

        bool isDebugEnabled() const noexcept;
        bool hasViableAdapter() const noexcept;

        Adapter *getAdapterByLUID(LUID luid) noexcept;
        Adapter& getWarpAdapter() noexcept;
        Adapter& getDefaultAdapter() noexcept;
    };

    void saveAdapterInfo(const Instance& instance, DXGI_FORMAT format, db::Connection& connection);
} // namespace sm::render

template<>
struct fmt::formatter<sm::render::AdapterLUID> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    constexpr auto format(const sm::render::AdapterLUID& luid, fmt::format_context& ctx) const {
        return format_to(ctx.out(), "{:x}:{:x}", luid.high, luid.low);
    }
};
