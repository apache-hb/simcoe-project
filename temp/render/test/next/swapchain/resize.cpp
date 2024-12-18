#include "render_test_common.hpp"

#include <random>

using namespace sm;
using namespace sm::math;

namespace next = sm::render::next;

using render::next::DebugFlags;
using render::next::FeatureLevel;

TEST_CASE("swapchain resize") {
    VirtualContextTest test{0};
    auto& context = test.getContext();
    SUCCEED("Created CoreContext successfully");

    auto limits = test.virtualSwapChain.limits();
    math::uint2 minSize = limits.minSize;
    math::uint2 maxSize = limits.maxSize;

    auto setSurfaceSize = [&](math::uint2 size) {
        context.updateSwapChain(newSurfaceInfo(size, 4));
    };

    // seeded rng for reproducibility
    std::mt19937 rng{};
    rng.seed(1234);
    std::uniform_int_distribution<UINT> distX(minSize.x, maxSize.x);
    std::uniform_int_distribution<UINT> distY(minSize.y, maxSize.y);

    for (int i = 0; i < 10; i++) {
        test.event(i, [&] {
            setSurfaceSize({ distX(rng), distY(rng) });
        });
    }

    test.run();

    SUCCEED("Ran CoreContext virtual swapchain test");
}
