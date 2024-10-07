#include "draw/camera.hpp"
#include "test/common.hpp"

#include "render/render.hpp"
#include "draw/draw.hpp"

using namespace sm;
using namespace sm::math;
using namespace sm::math::literals;

static constexpr system::WindowConfig kWindowConfig = {
    .mode = system::WindowMode::eWindowed,
    .width = 1920,
    .height = 1080,
    .title = "Test Window",
};

struct TestContext final : public render::IDeviceContext {
    using Super = render::IDeviceContext;
    using Super::Super;

    flecs::world world;

    draw::ecs::ViewportDeviceData viewportData;

    void setup_framegraph(graph::FrameGraph& graph) override {
        auto windowSize = getSwapChainSize();

        world::ecs::Camera cameraData {
            .colour = DXGI_FORMAT_R8G8B8A8_UNORM,
            .depth = DXGI_FORMAT_D32_FLOAT,
            .window = windowSize,
            .fov = 75._rad,
        };

        viewportData = render::newConstBuffer<sm::draw::ViewportData>(*this);
        draw::ecs::WorldData wd {
            world,
            "0", cameraData, viewportData,
        };
        draw::ecs::DrawData dd { draw::DepthBoundsMode::eEnabled, graph };

        draw::host::CameraData camera {
            flecs::string_view{"0"}, cameraData,
            { float3(-3.f, 0.f, 0.f) },
            { world::kVectorForward }
        };

        camera.updateViewportData(viewportData, 1, 1);

        graph::Handle spotLightVolumes, pointLightVolumes, spotLightData, pointLightData;
        graph::Handle depthPrePassTarget;
        graph::Handle lightIndices;
        graph::Handle forwardPlusOpaqueTarget, forwardPlusOpaqueDepth;
        draw::ecs::copyLightData(dd, spotLightVolumes, pointLightVolumes, spotLightData, pointLightData);
        draw::ecs::depthPrePass(wd, dd, depthPrePassTarget);
        draw::ecs::lightBinning(wd, dd, lightIndices, depthPrePassTarget, pointLightVolumes, spotLightVolumes);
        draw::ecs::forwardPlusOpaque(wd, dd, lightIndices, pointLightVolumes, spotLightVolumes, pointLightData, spotLightData, forwardPlusOpaqueTarget, forwardPlusOpaqueDepth);

        render::Viewport vp = render::Viewport::letterbox(windowSize, windowSize);

        draw::blit(graph, mSwapChainHandle, forwardPlusOpaqueTarget, vp);
    }

    TestContext(const render::RenderConfig& config)
        : Super(config)
    {
        world::ecs::initSystems(world);
        draw::ecs::initSystems(world, *this);
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
    // mask to only one l3 cache to improve warp performance
    SetProcessAffinityMask(GetCurrentProcess(), 0b1111'1111'1111'1111);

    system::create(GetModuleHandleA(nullptr));

    TestWindowEvents events;
    system::Window window{kWindowConfig, events};
    window.showWindow(system::ShowWindow::eShow);
    auto client = window.getClientCoords().size();

    sm::Bundle bundle{sm::mountArchive("bundle.tar")};

    render::RenderConfig renderConfig {
        .flags = render::DebugFlags::eDeviceDebugLayer
               | render::DebugFlags::eFactoryDebug
               | render::DebugFlags::eInfoQueue
               | render::DebugFlags::eGpuValidation
               | render::DebugFlags::eWarpAdapter,
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

    auto& world = context.world;

    flecs::entity floor = world.entity("Floor")
        .set<world::ecs::Position, world::ecs::Local>({ float3(0.f, 0.f, 0.f) })
        .set<world::ecs::Rotation>({ quatf::identity() })
        .set<world::ecs::Scale>({ 1.f })
        .add<world::ecs::Object>()
        .set<world::ecs::Shape>({ world::Cube{ 15.f, 1.f, 15.f } });

    world.entity("Player")
        .child_of(floor)
        .set<world::ecs::Position, world::ecs::Local>({ float3(0.f, 1.f, 0.f) })
        .set<world::ecs::Rotation>({ quatf::identity() })
        .set<world::ecs::Scale>({ 1.f })
        .add<world::ecs::Object>()
        .set<world::ecs::Shape>({ world::Cylinder{ 0.7f, 1.3f, 8 } });

    world.entity("Point Light")
        .set<world::ecs::Position, world::ecs::Local>({ float3(0.f, 5.f, 0.f) })
        .set<world::ecs::Colour>({ float3(1.f, 1.f, 1.f) })
        .set<world::ecs::Intensity>({ 1.f })
        .add<world::ecs::PointLight>();

    world.progress(1.f);

    bool done = false;
    const int totalIters = 20;
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

        context.render();

        if (iters == (totalIters / 2)) {
            window.resize({ 800, 600 });
        }

        if (--iters == 0) {
            done = true;
        }
    }

    context.destroy();

    SUCCEED();
}
