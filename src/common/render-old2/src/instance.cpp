#include "common.hpp"

#include "core/reflect.hpp" // IWYU pragma: export
#include "render/render.hpp"

#include "logs/sink.inl" // IWYU pragma: export

using namespace sm;
using namespace sm::render;

void log_adapter(const Adapter& adapter, Sink& sink) {
    sink.info("|| video memory: {}", adapter.vidmem());
    sink.info("|| system memory: {}", adapter.sysmem());
    sink.info("|| shared memory: {}", adapter.sharedmem());
    sink.info("|| flags: {}", adapter.flags());
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
}

bool Instance::enum_by_preference() {
    Object<IDXGIFactory6> factory6;
    if (Result hr = mFactory.query(&factory6); !hr) {
        mSink.warn("failed to query factory6, update to v1803: {}", hr);
        return false;
    }

    mSink.info("query by {}", mAdapterSearch);

    IDXGIAdapter1 *adapter;
    for (UINT i = 0; factory6->EnumAdapterByGpuPreference(i, mAdapterSearch.as_facade(), IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
        auto& it = mAdapters.emplace_back(adapter);
        mSink.info("| adapter {}: {}", i, it.name());
        log_adapter(it, mSink);
    }
    return true;
}

void Instance::enum_warp_adapter() {
    IDXGIAdapter1 *adapter;
    SM_ASSERT_HR(mFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
    mWarpAdapter = adapter;

    mSink.info("| warp adapter: {}", mWarpAdapter.name());
    log_adapter(mWarpAdapter, mSink);
}

void Instance::enum_adapters() {
    IDXGIAdapter1 *adapter;
    for (UINT i = 0; mFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        auto& it = mAdapters.emplace_back(adapter);
        mSink.info("| adapter {}: {}", i, it.name());
        log_adapter(it, mSink);
    }
}

void Instance::enable_leak_tracking() {
    if (Result hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&mDebug))) {
        mDebug->EnableLeakTrackingForThread();
    } else {
        mSink.error("failed to enable dxgi debug interface: {}", hr);
    }
}

Instance::Instance(InstanceConfig config)
    : mSink(config.logger)
    , mFlags(config.flags)
    , mAdapterSearch(config.preference)
{
    bool debug = mFlags.test(DebugFlags::eFactoryDebug);
    const UINT flags = debug ? DXGI_CREATE_FACTORY_DEBUG : 0;
    SM_ASSERT_HR(CreateDXGIFactory2(flags, IID_PPV_ARGS(&mFactory)));

    if (debug) enable_leak_tracking();

    mSink.info("instance config");
    mSink.info("| flags: {}", config.flags);

    enum_warp_adapter();

    if (!enum_by_preference())
        enum_adapters();
}

Instance::~Instance() {
    if (mDebug) {
        mSink.info("reporting live dxgi/d3d objects");
        mDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
    }
}
