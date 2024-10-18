#include "test/common.hpp"

#include "render/next/context/core.hpp"
#include "render/next/surface/virtual.hpp"

#include <stb_image_write.h>

using namespace sm;

namespace next = sm::render::next;

using render::next::DebugFlags;
using render::next::FeatureLevel;

static next::SurfaceInfo newSurfaceInfo(math::uint2 size, UINT length = 2, math::float4 clearColour = { 0.0f, 0.2f, 0.4f, 1.0f }) {
    return next::SurfaceInfo {
        .format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .size = size,
        .length = length,
        .clearColour = clearColour,
    };
}

TEST_CASE("Create next::CoreContext virtual swapchain") {
    next::VirtualSwapChainFactory swapChainFactory { };

    std::filesystem::create_directories("test-data/render/frames");

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
        .swapChainInfo = newSurfaceInfo(math::uint2(720, 360), 2),
    };

    next::CoreContext context{config};
    SUCCEED("Created CoreContext successfully");

    render::AdapterLUID warpAdapter = context.getWarpAdapter().luid();

    const int totalIters = 15;
    for (int iters = 0; iters < totalIters; iters++) {
        next::VirtualSwapChain *swapchain = swapChainFactory.getSwapChain(context.getDevice());
        std::span<const uint32_t> image = swapchain->getImage();
        next::SurfaceInfo info = swapchain->getSurfaceInfo();

        context.present();

        std::string path = fmt::format("test-data/render/frames/frame-{}.png", iters);
        stbi_write_png(path.c_str(), info.size.width, info.size.height, 4, image.data(), 4 * info.size.width);

        if (iters == 10) {
            context.updateSwapChain(newSurfaceInfo(math::uint2(800, 600), 4));
            SUCCEED("Shrank swapchain resolution");
        }

        if (iters == 14) {
            CHECK_THROWS_AS(context.setAdapter(render::AdapterLUID(0x1000, 0x1234)), render::RenderException);
        }

        // remove the device first to ensure the context is in a bad state
        // if setAdapter leaks any device resources then the test will fail
        if (iters == 13) {
            render::AdapterLUID currentAdapter = context.getAdapter();
            context.removeDevice();
            context.setAdapter(currentAdapter);
            SUCCEED("recovered from device lost");
        }

        if (iters == 4) {
            context.setAdapter(warpAdapter);
            SUCCEED("Moved to warp adapter");
        }

        if (iters == 2) {
            context.updateSwapChain(newSurfaceInfo(math::uint2(1920, 1080), 4));
            SUCCEED("Updated swapchain size");
        }
    }

    SUCCEED("Ran CoreContext virtual swapchain test");
}
