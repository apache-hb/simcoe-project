#include "test/draw_test_common.hpp"

#include "draw/next/context.hpp"

namespace dn = sm::draw::next;

TEST_CASE("Dear ImGui DrawContext") {
    system::create(GetModuleHandle(nullptr));

    DrawWindowTestContext test{30};
    auto& context = test.getContext();

    test.event(14, [&] {
        CHECK_THROWS_AS(context.setAdapter(render::AdapterLUID(0x1000, 0x1234)), render::RenderException);
    });

    test.event(13, [&] {
        render::AdapterLUID currentAdapter = context.getAdapter();
        context.removeDevice();
        context.setAdapter(currentAdapter);
        SUCCEED("recovered from device lost");
    });

    test.event(10, [&] {
        context.updateSwapChain(newSurfaceInfo(math::uint2(800, 600), 4));
        SUCCEED("Shrank swapchain resolution");
    });

    test.event(4, [&] {
        render::AdapterLUID warpAdapter = context.getWarpAdapter().luid();
        context.setAdapter(warpAdapter);
        SUCCEED("Moved to warp adapter");
    });

    test.event(2, [&] {
        context.updateSwapChain(newSurfaceInfo(math::uint2(1920, 1080), 4));
        SUCCEED("Updated swapchain size");
    });

    while (test.next()) {
        context.begin();

        context.end();
    }

    if (test.frames.iters > 0) {
        FAIL("Test exited early");
    }

    SUCCEED("Ran CoreContext loop");
}
