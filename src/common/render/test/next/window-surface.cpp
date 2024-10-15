#include "test/common.hpp"
#include "test/db_test_common.hpp"

#include "db/connection.hpp"

#include "render/base/instance.hpp"
#include "render/next/context.hpp"

using namespace sm;

namespace next = sm::render::next;

using render::next::DebugFlags;
using render::next::FeatureLevel;

static next::SurfaceInfo newSurfaceInfo(math::uint2 size) {
    return next::SurfaceInfo {
        .format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .size = size,
        .length = 2,
        .clearColour = { 0.0f, 0.0f, 0.0f, 1.0f },
    };
}

class TestWindowEvents final : public system::IWindowEvents {
    void resize(system::Window& window, math::int2 size) override {
        if (context == nullptr)
            return;

        next::SurfaceInfo info = newSurfaceInfo(math::uint2(size));
        context->updateSwapChain(info);
    }

public:
    next::CoreContext *context = nullptr;
};

TEST_CASE("Create next::CoreContext with window swapchain") {
    system::create(GetModuleHandle(nullptr));

    TestWindowEvents events { };
    system::Window window {
        system::WindowConfig {
            .mode = system::WindowMode::eWindowed,
            .width = 800, .height = 600,
            .title = "Test"
        },
        events
    };
    window.showWindow(system::ShowWindow::eShow);

    auto client = window.getClientCoords().size();

    next::WindowSwapChainFactory swapChainFactory { window.getHandle() };

    next::ContextConfig config {
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
        .swapChainFactory = &swapChainFactory,
        .swapChainInfo = {
            .format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .size = client,
            .length = 2,
            .clearColour = { 0.0f, 0.0f, 0.0f, 1.0f },
        },
    };

    next::CoreContext context{config};
    SUCCEED("Created CoreContext successfully");
    events.context = &context;

    render::AdapterLUID warpAdapter = context.getWarpAdapter().luid();

    bool done = false;
    const int totalIters = 60;
    int iters = totalIters;
    while (!done) {
        MSG msg = {};
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
            if (msg.message == WM_QUIT) {
                done = true;
            }
        }

        if (done) break;

        context.present();

        if (iters == 30) {
            window.resize({ 400, 300 });
        }

        if (iters == 45) {
            CHECK_THROWS_AS(context.setAdapter(render::AdapterLUID(0x1000, 0x1234)), render::RenderException);
        }

        if (iters == 40) {
            context.setAdapter(warpAdapter);
        }

        if (--iters == 0) {
            done = true;
        }
    }

    SUCCEED("Ran CoreContext loop");
}
