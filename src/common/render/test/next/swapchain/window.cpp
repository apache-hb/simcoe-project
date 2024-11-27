#include "render_test_common.hpp"

using namespace sm;

namespace next = sm::render::next;

using render::next::DebugFlags;
using render::next::FeatureLevel;

TEST_CASE("Create next::CoreContext with window swapchain") {
    system::create(GetModuleHandle(nullptr));

    WindowContextTest test{60};
    auto& context = test.getContext();

    SUCCEED("Created CoreContext successfully");

    test.event(30, [&] {
        test.window.setSize({ 400, 300 });
    });

    test.event(45, [&] {
        CHECK_THROWS_AS(context.setAdapter(render::AdapterLUID(0x1000, 0x1234)), render::RenderException);
    });

    test.event(43, [&] {
        render::AdapterLUID currentAdapter = context.getAdapter();
        context.removeDevice();
        context.setAdapter(currentAdapter);
        SUCCEED("recovered from device lost");
    });

    test.event(40, [&] {
        render::AdapterLUID warpAdapter = context.getWarpAdapter().luid();
        context.setAdapter(warpAdapter);
        SUCCEED("Moved to warp adapter");
    });

    test.event(35, [&] {
        context.updateSwapChain(newSurfaceInfo(math::uint2(1920, 1080), 4));
        SUCCEED("Updated swapchain size");
    });

    test.run();

    SUCCEED("Ran CoreContext loop");
}
