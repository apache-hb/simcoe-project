#include "render_test_common.hpp"

#include <stb_image_write.h>

using namespace sm;

TEST_CASE("Create next::CoreContext virtual swapchain") {
    VirtualContextTest test{0};
    next::CoreContext& context = test.getContext();
    SUCCEED("Created CoreContext successfully");
    std::filesystem::create_directories("test-data/render/frames");

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

    while (test.frames.iters > 0) {
        next::VirtualSwapChain *swapchain = test.virtualSwapChain.getSwapChain(context.getDevice());
        std::span<const uint32_t> image = swapchain->getImage();
        next::SurfaceInfo info = swapchain->getSurfaceInfo();

        context.present();

        // ensure the image is the same as the clear colour
        REQUIRE(image.size() == (info.size.width * info.size.height));
        for (size_t i = 0; i < info.size.width * info.size.height; i++) {

            // if any pixel isnt cleared, write the image to disk for inspection
            if (image[i] != 0xff663300) {
                std::string path = fmt::format("test-data/render/frames/frame-{}.png", test.frames.iters);
                stbi_write_png(path.c_str(), info.size.width, info.size.height, 4, image.data(), 4 * info.size.width);

                CHECK(image[i] == 0xff663300);
                break;
            }
        }

        test.doEvent();
        test.frames.iters--;
    }

    SUCCEED("Ran CoreContext virtual swapchain test");
}
