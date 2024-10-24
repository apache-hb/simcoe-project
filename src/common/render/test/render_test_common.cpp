#include "render_test_common.hpp"

next::SurfaceInfo newSurfaceInfo(math::uint2 size, UINT length, math::float4 clearColour) {
    return next::SurfaceInfo {
        .format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .size = size,
        .length = length,
        .clearColour = clearColour,
    };
}

next::ContextConfig newConfig(next::ISwapChainFactory *factory, math::uint2 size, bool debug) {
    const DebugFlags kAllFlags
        = DebugFlags::eDeviceDebugLayer
        | DebugFlags::eFactoryDebug
        | DebugFlags::eDeviceRemovedInfo
        | DebugFlags::eInfoQueue
        | DebugFlags::eAutoName
        | DebugFlags::eGpuValidation
        | DebugFlags::eDirectStorageDebug
        | DebugFlags::eDirectStorageBreak
        | DebugFlags::eDirectStorageNames
        | DebugFlags::eWinPixEventRuntime;

    return next::ContextConfig {
        .flags = debug ? kAllFlags : DebugFlags::eNone,
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
