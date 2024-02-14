#include "common.hpp"

#include "core/reflect.hpp" // IWYU pragma: export
#include "render/render.hpp"

#include "logs/sink.inl" // IWYU pragma: export

using namespace sm;
using namespace sm::render;

void Instance::enum_warp_adapter() {
    IDXGIAdapter1 *adapter;
    SM_ASSERT_HR(mFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));

    DXGI_ADAPTER_DESC1 desc;
    SM_ASSERT_HR(adapter->GetDesc1(&desc));

    mAdapters.emplace_back(adapter);
    mSink.info("| warp adapter: {}", sm::narrow(desc.Description));
}

void Instance::enum_adapters() {
    IDXGIAdapter1 *adapter;
    for (UINT i = 0; mFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc;
        SM_ASSERT_HR(adapter->GetDesc1(&desc));

        mAdapters.emplace_back(adapter);
        mSink.info("| adapter {}: {}", i, sm::narrow(desc.Description));
    }
}

Instance::Instance(InstanceConfig config)
    : mSink(config.logger)
{
    bool debug = config.flags.test(DebugFlags::eFactoryDebug);
    const UINT flags = debug ? DXGI_CREATE_FACTORY_DEBUG : 0;
    SM_ASSERT_HR(CreateDXGIFactory2(flags, IID_PPV_ARGS(&mFactory)));

    if (debug) {
        if (Result hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&mDebug)); hr) {
            mDebug->EnableLeakTrackingForThread();
        } else {
            mSink.error("failed to enable dxgi debug interface: {}", hr);
        }
    }

    mSink.info("instance config");
    mSink.info("| flags: {}", config.flags);

    bool warp = config.flags.test(DebugFlags::eWarpAdapter);

    mSink.info("enumerating {} {}", warp ? "warp" : "hardware", warp ? "adapter" : "adapters");

    if (warp) {
        enum_warp_adapter();
    } else {
        enum_adapters();
    }
}
