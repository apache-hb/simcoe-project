#include "render/instance.hpp"

#include "core/format.hpp" // IWYU pragma: export

using namespace sm;
using namespace sm::render;

static auto gSink = logs::get_sink(logs::Category::eRHI);

void log_adapter(const Adapter &adapter) {
    gSink.info("|| video memory: {}", adapter.vidmem());
    gSink.info("|| system memory: {}", adapter.sysmem());
    gSink.info("|| shared memory: {}", adapter.sharedmem());
    gSink.info("|| flags: {}", adapter.flags());
}

Adapter::Adapter(IDXGIAdapter1 *adapter)
    : Object(adapter) {
    DXGI_ADAPTER_DESC1 desc;
    SM_ASSERT_HR(adapter->GetDesc1(&desc));

    mName = sm::narrow(desc.Description);
    mVideoMemory = desc.DedicatedVideoMemory;
    mSystemMemory = desc.DedicatedSystemMemory;
    mSharedMemory = desc.SharedSystemMemory;
    mFlags = desc.Flags;
}

bool Instance::enum_by_preference() {
    Object<IDXGIFactory6> factory6;
    if (Result hr = mFactory.query(&factory6); !hr) {
        gSink.warn("failed to query factory6: {}", hr);
        return false;
    }

    gSink.info("querying for {} adapter", mAdapterSearch);

    IDXGIAdapter1 *adapter;
    for (UINT i = 0;
         factory6->EnumAdapterByGpuPreference(i, mAdapterSearch.as_facade(),
                                              IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND;
         ++i) {
        auto &it = mAdapters.emplace_back(adapter);
        gSink.info("| adapter {}: {}", i, it.name());
        log_adapter(it);
    }
    return true;
}

void Instance::enum_adapters() {
    IDXGIAdapter1 *adapter;
    for (UINT i = 0; mFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        auto &it = mAdapters.emplace_back(adapter);
        gSink.info("| adapter {}: {}", i, it.name());
        log_adapter(it);
    }
}

void Instance::enable_leak_tracking() {
    if (Result hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&mDebug))) {
        mDebug->EnableLeakTrackingForThread();
    } else {
        gSink.error("failed to enable dxgi debug interface: {}", hr);
    }
}

void Instance::query_tearing_support() {
    Object<IDXGIFactory5> factory;
    if (Result hr = mFactory.query(&factory); !hr) {
        gSink.warn("failed to query factory5: {}", hr);
        return;
    }

    BOOL tearing = FALSE;
    if (Result hr = factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearing, sizeof(tearing)); !hr) {
        gSink.warn("failed to query tearing support: {}", hr);
        return;
    }

    mTearingSupport = tearing;
}

Instance::Instance(InstanceConfig config)
    : mFlags(config.flags)
    , mAdapterSearch(config.preference)
{
    bool debug = mFlags.test(DebugFlags::eFactoryDebug);
    const UINT flags = debug ? DXGI_CREATE_FACTORY_DEBUG : 0;
    SM_ASSERT_HR(CreateDXGIFactory2(flags, IID_PPV_ARGS(&mFactory)));

    query_tearing_support();
    gSink.info("tearing support: {}", mTearingSupport);

    if (debug) enable_leak_tracking();

    gSink.info("instance config");
    gSink.info("| flags: {}", config.flags);

    if (!enum_by_preference()) enum_adapters();
}

Instance::~Instance() {
    if (mDebug) {
        gSink.info("reporting live dxgi/d3d objects");
        mDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
    }
}

size_t Instance::warp_adapter_index() {
    for (size_t i = 0; i < mAdapters.size(); i++) {
        if (mAdapters[i].flags().test(AdapterFlag::eSoftware))
            return i;
    }
    return SIZE_MAX;
}

Adapter &Instance::get_adapter(size_t index) {
    return mAdapters[index];
}

sm::Vector<Adapter> &Instance::get_adapters() {
    return mAdapters;
}

Object<IDXGIFactory4> &Instance::factory() {
    return mFactory;
}

const DebugFlags &Instance::flags() const {
    return mFlags;
}

bool Instance::tearing_support() const {
    return mTearingSupport;
}
