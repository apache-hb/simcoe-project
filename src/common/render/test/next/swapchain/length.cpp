#include "render_test_common.hpp"

#include "render/next/context/core.hpp"
#include "render/next/surface/virtual.hpp"

#include <random>

using namespace sm;

namespace next = sm::render::next;

using render::next::DebugFlags;
using render::next::FeatureLevel;

TEST_CASE("swapchain length") {
    VirtualContextTest test{0};
    auto& context = test.getContext();
    SUCCEED("Created CoreContext successfully");

    auto limits = test.virtualSwapChain.limits();
    UINT minLength = limits.minLength;
    UINT maxLength = limits.maxLength;

    auto setSurfaceLength = [&](UINT length) {
        context.updateSwapChain(newSurfaceInfo(math::uint2(800, 600), length));
    };

    for (UINT i = minLength; i < maxLength; i++) {
        test.event(i, [&] {
            setSurfaceLength(i);
        });
    }

    for (UINT i = maxLength; i > minLength; i--) {
        test.event(i + maxLength, [&] {
            setSurfaceLength(i);
        });
    }

    // seeded rng for reproducibility
    std::mt19937 rng{};
    rng.seed(1234);
    std::uniform_int_distribution<int> dist(minLength, maxLength);

    for (int i = 0; i < 10; i++) {
        test.event(1 + i + (maxLength * 2), [&] {
            setSurfaceLength(dist(rng));
        });
    }

    test.run();

    SUCCEED("Ran CoreContext virtual swapchain test");
}
