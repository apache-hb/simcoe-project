#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include "test/common.hpp"

#include "render/render.hpp"

using namespace sm;

static constexpr system::WindowConfig kWindowConfig = {
    .mode = system::WindowMode::eWindowed,
    .width = 1280,
    .height = 720,
    .title = "Test Window",
};

struct TestContext final : public render::IDeviceContext {
    using Super = render::IDeviceContext;
    using Super::Super;

    void setup_framegraph(graph::FrameGraph& graph) override { }
};

class TestWindowEvents final : public system::IWindowEvents {

};

TEST_CASE("Creating a device context") {
    system::create(GetModuleHandleA(nullptr));

    TestWindowEvents events;
    system::Window window{kWindowConfig, events};
    window.showWindow(system::ShowWindow::eShow);
    auto client = window.getClientCoords().size();

    render::RenderConfig renderConfig {
        .preference = render::AdapterPreference::eMinimumPower,
        .minFeatureLevel = render::FeatureLevel::eLevel_11_0,

        .swapchain = {
            .size = math::uint2(client),
            .length = 2,
            .format = DXGI_FORMAT_R8G8B8A8_UNORM,
        },

        .rtvHeapSize = 64,
        .dsvHeapSize = 64,
        .srvHeapSize = 1024,

        .window = window,
    };

    TestContext context{renderConfig};
    context.create();

    bool done = false;
    int iters = 10;
    while (!done) {
        MSG msg = {};
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
            if (msg.message == WM_QUIT) {
                done = true;
            }
        }

        if (iters-- == 0) break;

        if (done) break;

        context.render();
    }

    context.destroy();

    SUCCEED();
}
