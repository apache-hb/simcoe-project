#include "stdafx.hpp"

#include "render/instance.hpp"

using namespace sm;
using namespace sm::render;

LOG_CATEGORY_IMPL(gGpuLog, "gpu");

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
    : Object(adapter)
{
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

bool Instance::enumAdaptersByPreference() {
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

void Instance::enumAdapters() {
    IDXGIAdapter1 *adapter;
    for (UINT i = 0; mFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        auto &it = mAdapters.emplace_back(adapter);
        gGpuLog.info("| adapter {}: {}", i, it.name());
        logAdapterInfo(it);
    }
}

void Instance::findWarpAdapter() {
    IDXGIAdapter1 *adapter;
    if (Result hr = mFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)); !hr) {
        gGpuLog.warn("failed to enum warp adapter: {}", hr);
        return;
    }

    mWarpAdapter = Adapter(adapter);
    gGpuLog.info("warp adapter: {}", mWarpAdapter.name());
    logAdapterInfo(mWarpAdapter);
}

void Instance::enableDebugLeakTracking() {
    if (Result hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&mDebug))) {
        mDebug->EnableLeakTrackingForThread();
    } else {
        gGpuLog.error("failed to enable dxgi debug interface: {}", hr);
    }
}

void Instance::queryTearingSupport() {
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

void Instance::loadWarpRedist() {
#if SMC_WARP_ENABLE
    mWarpLibrary = sm::get_redist("d3d10warp.dll");
    if (OsError err = mWarpLibrary.get_error()) {
        gGpuLog.warn("failed to load warp redist: {}", err);
    } else {
        gGpuLog.info("loaded warp redist");
    }
#endif
}

void Instance::loadPIXRuntime() {
#if SMC_PIX_ENABLE
    if (PIXLoadLatestWinPixGpuCapturerLibrary()) {
        gGpuLog.info("loaded pix runtime");
    } else {
        gGpuLog.warn("failed to load pix runtime: {}", sys::getLastError());
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

    queryTearingSupport();
    gGpuLog.info("tearing support: {}", mTearingSupport);

    if (debug)
        enableDebugLeakTracking();

    gGpuLog.info("instance config");
    gGpuLog.info("| flags: {}", mFlags);

    loadWarpRedist();

    if (mFlags.test(DebugFlags::eWinPixEventRuntime)) {
        loadPIXRuntime();
    }

    findWarpAdapter();

    if (!enumAdaptersByPreference())
        enumAdapters();
}

Instance::~Instance() {
    if (mDebug) {
        gGpuLog.info("reporting live dxgi/d3d objects");
        mDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
    }
}

std::span<Adapter> Instance::adapters() noexcept {
    return mAdapters;
}

bool Instance::hasViableAdapter() const noexcept {
    return mWarpAdapter.isValid() || !mAdapters.empty();
}

Adapter *Instance::getAdapterByLUID(LUID luid) noexcept {
    for (auto &adapter : mAdapters) {
        if (adapter.luid() == luid)
            return std::addressof(adapter);
    }
    return nullptr;
}

Adapter& Instance::getWarpAdapter() noexcept {
    return mWarpAdapter;
}

Adapter& Instance::getDefaultAdapter() noexcept {
    return mAdapters.empty() ? getWarpAdapter() : mAdapters.front();
}

Object<IDXGIFactory4> &Instance::factory() noexcept {
    return mFactory;
}

const DebugFlags &Instance::flags() const noexcept {
    return mFlags;
}

bool Instance::isTearingSupported() const noexcept {
    return mTearingSupport;
}

bool Instance::isDebugEnabled() const noexcept {
    return mFlags.test(DebugFlags::eFactoryDebug);
}
