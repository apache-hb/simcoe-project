#include "draw/camera.hpp"
#include "test/common.hpp"

#include "render/render.hpp"
#include "draw/draw.hpp"

using namespace sm;
using namespace sm::math::literals;

static constexpr system::WindowConfig kWindowConfig = {
    .mode = system::WindowMode::eWindowed,
    .width = 1280,
    .height = 720,
    .title = "Test Window",
};

struct TestContext final : public render::IDeviceContext {
    using Super = render::IDeviceContext;
    using Super::Super;

    draw::ViewportConfig viewport = {
        .size = { 1920, 1080 },
        .colour = DXGI_FORMAT_R8G8B8A8_UNORM,
        .depth = DXGI_FORMAT_D32_FLOAT,
    };

    draw::Camera camera{"client", viewport};

    flecs::world world;

    draw::ecs::ViewportDeviceData viewportData;

    void setup_framegraph(graph::FrameGraph& graph) override {
        viewportData = render::newConstBuffer<sm::draw::ViewportData>(*this);
        world::ecs::Camera cameraData {
            .colour = viewport.colour,
            .depth = viewport.depth,
            .window = viewport.size,
            .fov = 75._rad,
        };
        draw::ecs::WorldData wd {
            world,
            "0", cameraData, viewportData,
        };
        draw::ecs::DrawData dd { draw::DepthBoundsMode::eEnabled, graph };

        graph::Handle spotLightVolumes, pointLightVolumes, spotLightData, pointLightData;
        graph::Handle depthPrePassTarget;
        graph::Handle lightIndices;
        graph::Handle forwardPlusOpaqueTarget, forwardPlusOpaqueDepth;
        draw::ecs::copyLightData(dd, spotLightVolumes, pointLightVolumes, spotLightData, pointLightData);
        draw::ecs::depthPrePass(wd, dd, depthPrePassTarget);
        draw::ecs::lightBinning(wd, dd, lightIndices, depthPrePassTarget, pointLightVolumes, spotLightVolumes);
        draw::ecs::forwardPlusOpaque(wd, dd, lightIndices, pointLightVolumes, spotLightVolumes, pointLightData, spotLightData, forwardPlusOpaqueTarget, forwardPlusOpaqueDepth);

        render::Viewport vp = render::Viewport::letterbox(getSwapChainSize(), viewport.size);

        draw::blit(graph, mSwapChainHandle, forwardPlusOpaqueTarget, vp);
    }
};

class TestWindowEvents final : public system::IWindowEvents {
    void resize(system::Window& window, math::int2 size) override {
        if (context != nullptr)
            context->resize_swapchain(math::uint2(size));
    }

public:
    render::IDeviceContext *context = nullptr;
};

TEST_CASE("Creating a device context") {
    system::create(GetModuleHandleA(nullptr));

    TestWindowEvents events;
    system::Window window{kWindowConfig, events};
    window.showWindow(system::ShowWindow::eShow);
    auto client = window.getClientCoords().size();

    sm::Bundle bundle{sm::mountArchive("bundle.tar")};

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

        .bundle = &bundle,
        .window = window,
    };

    TestContext context{renderConfig};
    events.context = &context;
    context.create();

    bool done = false;
    int iters = 5;
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

        context.render();

        if (iters == 4) {
            window.resize({ 800, 600 });
        }

        if (--iters == 0) {
            done = true;
        }
    }

    context.destroy();

    SUCCEED();
}
