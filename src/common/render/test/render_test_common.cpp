#include "render_test_common.hpp"

next::SurfaceInfo newSurfaceInfo(math::uint2 size, UINT length, math::float4 clearColour) {
    return next::SurfaceInfo {
        .format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .size = size,
        .length = length,
        .clearColour = clearColour,
    };
}

static next::ContextConfig newConfig(next::ISwapChainFactory *factory, math::uint2 size) {
    return next::ContextConfig {
        .flags = DebugFlags::eDeviceDebugLayer
               | DebugFlags::eFactoryDebug
               | DebugFlags::eDeviceRemovedInfo
               | DebugFlags::eInfoQueue
               | DebugFlags::eAutoName
               | DebugFlags::eGpuValidation
               | DebugFlags::eDirectStorageDebug
               | DebugFlags::eDirectStorageBreak
               | DebugFlags::eDirectStorageNames
               | DebugFlags::eWinPixEventRuntime,
        .targetLevel = FeatureLevel::eLevel_11_0,
        .swapChainFactory = factory,
        .swapChainInfo = newSurfaceInfo(size, 2),
    };
}

ContextTest::ContextTest(int frames, math::uint2 size, next::ISwapChainFactory *factory)
    : swapChainFactory(factory)
    , context(newConfig(swapChainFactory, size))
    , iters(frames)
{ }

int ContextTest::next() {
    context.present();

    doEvent();

    return iters--;
}

void ContextTest::doEvent() {
    auto action = events.find(iters);
    if (action != events.end()) {
        action->second();
    }
}

static system::WindowConfig newWindowConfig(const char *title) {
    return system::WindowConfig {
        .mode = system::WindowMode::eWindowed,
        .width = 800, .height = 600,
        .title = title
    };
}

WindowContextTest::WindowContextTest(int frames)
    : name(sm::system::getProgramName())
    , window(newWindowConfig(name.c_str()), events)
    , windowSwapChain(window.getHandle())
    , context(frames, window.getClientCoords().size(), &windowSwapChain)
{
    window.showWindow(system::ShowWindow::eShow);
    events.context = &context.context;
}

bool WindowContextTest::next() {
    MSG msg = {};
    bool done = false;
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
        if (msg.message == WM_QUIT) {
            done = true;
        }
    }

    return !done && context.iters-- > 0;
}
