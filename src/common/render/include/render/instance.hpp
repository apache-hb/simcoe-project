#pragma once

#include "render.reflect.h"

namespace sm::render {

struct InstanceConfig {
    DebugFlags flags;
    AdapterPreference preference;
    logs::ILogger &logger;
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
};

class Instance {
    Sink mSink;

    DebugFlags mFlags;
    AdapterPreference mAdapterSearch;

    Object<IDXGIFactory4> mFactory;
    Object<IDXGIDebug1> mDebug;
    sm::Vector<Adapter> mAdapters;
    Adapter mWarpAdapter;

    void enable_leak_tracking();

public:
    Instance(InstanceConfig config);
    ~Instance();

    bool enum_by_preference();
    void enum_warp_adapter();
    void enum_adapters();

    Adapter &warp_adapter() {
        return mWarpAdapter;
    }
    Adapter &get_adapter(size_t index) {
        return mAdapters[index];
    }

    Object<IDXGIFactory4> &factory() {
        return mFactory;
    }

    const DebugFlags &flags() const {
        return mFlags;
    }
};

} // namespace sm::render
