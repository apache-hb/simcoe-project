#pragma once

#include "core/vector.hpp"
#include "core/memory.hpp"

#include "render.reflect.h"

namespace sm::render {
struct InstanceConfig {
    DebugFlags flags;
    AdapterPreference preference;
};

class Adapter : public Object<IDXGIAdapter1> {
    sm::String mName;
    sm::Memory mVideoMemory{0};
    sm::Memory mSystemMemory{0};
    sm::Memory mSharedMemory{0};
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
};

class Instance {
    DebugFlags mFlags;
    AdapterPreference mAdapterSearch;

    Object<IDXGIFactory4> mFactory;
    Object<IDXGIDebug1> mDebug;
    sm::Vector<Adapter> mAdapters;
    bool mTearingSupport = false;

    void enable_leak_tracking();
    void query_tearing_support();

    bool enum_by_preference();
    void enum_adapters();

public:
    Instance(InstanceConfig config);
    ~Instance();

    size_t warp_adapter_index();
    Adapter &get_adapter(size_t index);
    sm::Vector<Adapter> &get_adapters();
    Object<IDXGIFactory4> &factory();
    const DebugFlags &flags() const;
    bool tearing_support() const;
};

} // namespace sm::render
