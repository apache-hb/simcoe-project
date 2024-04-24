#include "stdafx.hpp"

#include "render/instance.hpp"

using namespace sm;
using namespace sm::render;

LOG_CATEGORY_IMPL(sm::gGpuLog, "gpu");

static void logAdapterInfo(const Adapter &adapter) {
    gGpuLog.info("|| video memory: {}", adapter.vidmem());
    gGpuLog.info("|| system memory: {}", adapter.sysmem());
    gGpuLog.info("|| shared memory: {}", adapter.sharedmem());

    auto [high, low] = adapter.luid();
    gGpuLog.info("|| luid: high: {}, low: {}", high, low);

    auto [vendor, device, subsystem, revision] = adapter.info();
    gGpuLog.info("|| device info: vendor: {}, device: {}, subsystem: {}, revision: {}",
                 vendor, device, subsystem, revision);

    gGpuLog.info("|| flags: {}", adapter.flags());
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
    mAdapterLuid = desc.AdapterLuid;

    mDeviceInfo = {
        .vendor = desc.VendorId,
        .device = desc.DeviceId,
        .subsystem = desc.SubSysId,
        .revision = desc.Revision,
    };
}

bool Instance::enum_by_preference() {
    Object<IDXGIFactory6> factory6;
    if (Result hr = mFactory.query(&factory6); !hr) {
        gGpuLog.warn("failed to query factory6: {}", hr);
        return false;
    }

    gGpuLog.info("querying for {} adapter", mAdapterSearch);

    IDXGIAdapter1 *adapter;
    for (UINT i = 0;
         factory6->EnumAdapterByGpuPreference(i, mAdapterSearch.as_facade(),
                                              IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND;
         ++i) {
        auto &it = mAdapters.emplace_back(adapter);
        gGpuLog.info("| adapter {}: {}", i, it.name());
        logAdapterInfo(it);
    }
    return true;
}

void Instance::enum_adapters() {
    IDXGIAdapter1 *adapter;
    for (UINT i = 0; mFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        auto &it = mAdapters.emplace_back(adapter);
        gGpuLog.info("| adapter {}: {}", i, it.name());
        logAdapterInfo(it);
    }
}

void Instance::enum_warp_adapter() {
    IDXGIAdapter1 *adapter;
    if (Result hr = mFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)); !hr) {
        gGpuLog.warn("failed to enum warp adapter: {}", hr);
        return;
    }

    mWarpAdapter = Adapter(adapter);
    gGpuLog.info("warp adapter: {}", mWarpAdapter.name());
    logAdapterInfo(mWarpAdapter);
}

void Instance::enable_leak_tracking() {
    if (Result hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&mDebug))) {
        mDebug->EnableLeakTrackingForThread();
    } else {
        gGpuLog.error("failed to enable dxgi debug interface: {}", hr);
    }
}

void Instance::query_tearing_support() {
    Object<IDXGIFactory5> factory;
    if (Result hr = mFactory.query(&factory); !hr) {
        gGpuLog.warn("failed to query factory5: {}", hr);
        return;
    }

    BOOL tearing = FALSE;
    if (Result hr = factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearing, sizeof(tearing)); !hr) {
        gGpuLog.warn("failed to query tearing support: {}", hr);
        return;
    }

    mTearingSupport = tearing;
}

void Instance::load_warp_redist() {
#if SMC_WARP_ENABLE
    mWarpLibrary = sm::get_redist("d3d10warp.dll");
    if (OsError err = mWarpLibrary.get_error()) {
        gGpuLog.warn("failed to load warp redist: {}", err);
    } else {
        gGpuLog.info("loaded warp redist");
    }
#endif
}

void Instance::load_pix_runtime() {
#if SMC_PIX_ENABLE
    if (PIXLoadLatestWinPixGpuCapturerLibrary()) {
        gGpuLog.info("loaded pix runtime");
    } else {
        gGpuLog.warn("failed to load pix runtime: {}", sys::get_last_error());
    }
#endif
}

Instance::Instance(InstanceConfig config)
    : mFlags(config.flags)
    , mAdapterSearch(config.preference)
{
    bool debug = mFlags.test(DebugFlags::eFactoryDebug);
    const UINT flags = debug ? DXGI_CREATE_FACTORY_DEBUG : 0;
    SM_ASSERT_HR(CreateDXGIFactory2(flags, IID_PPV_ARGS(&mFactory)));

    query_tearing_support();
    gGpuLog.info("tearing support: {}", mTearingSupport);

    if (debug)
        enable_leak_tracking();

    gGpuLog.info("instance config");
    gGpuLog.info("| flags: {}", mFlags);

    load_warp_redist();

    if (mFlags.test(DebugFlags::eWinPixEventRuntime)) {
        load_pix_runtime();
    }

    enum_warp_adapter();

    if (!enum_by_preference())
        enum_adapters();
}

Instance::~Instance() {
    if (mDebug) {
        gGpuLog.info("reporting live dxgi/d3d objects");
        mDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
    }
}

sm::Vector<Adapter> &Instance::get_adapters() {
    return mAdapters;
}

bool Instance::has_viable_adapter() const {
    return mWarpAdapter.is_valid() || !mAdapters.empty();
}

Adapter *Instance::get_adapter_by_luid(LUID luid) {
    for (auto &adapter : mAdapters) {
        if (adapter.luid() == luid)
            return std::addressof(adapter);
    }
    return nullptr;
}

Adapter& Instance::get_warp_adapter() {
    return mWarpAdapter;
}

Adapter& Instance::get_default_adapter() {
    return mAdapters.empty() ? get_warp_adapter() : mAdapters.front();
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

bool Instance::debug_support() const {
    return mFlags.test(DebugFlags::eFactoryDebug);
}
