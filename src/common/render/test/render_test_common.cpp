#include "render_test_common.hpp"

next::SurfaceInfo newSurfaceInfo(math::uint2 size, UINT length, math::float4 clear) {
    return next::SurfaceInfo {
        .format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .size = size,
        .length = length,
        .clear = clear,
    };
}

next::ContextConfig newConfig(next::ISwapChainFactory *factory, math::uint2 size, bool debug) {
    DebugFlags allFlags
        = DebugFlags::eDeviceDebugLayer
        | DebugFlags::eFactoryDebug
        | DebugFlags::eDeviceRemovedInfo
        | DebugFlags::eInfoQueue
        | DebugFlags::eAutoName
        | DebugFlags::eGpuValidation
        | DebugFlags::eWarpAdapter // TODO: should this always be here?
        | DebugFlags::eDirectStorageDebug
        | DebugFlags::eDirectStorageBreak
        | DebugFlags::eDirectStorageNames
        | DebugFlags::eWinPixEventRuntime;
    
    if (!debug) {
        allFlags = DebugFlags::eWarpAdapter;
    }

    return next::ContextConfig {
        .flags = allFlags,
        .targetLevel = FeatureLevel::eLevel_11_0,
        .swapChainFactory = factory,
        .swapChainInfo = newSurfaceInfo(size, 2),
    };
}

system::WindowConfig newWindowConfig(const char *title) {
    return system::WindowConfig {
        .mode = system::WindowMode::eWindowed,
        .width = 800, .height = 600,
        .title = title
    };
}
