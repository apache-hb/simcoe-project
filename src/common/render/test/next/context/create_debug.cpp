#include "render_test_common.hpp"

using namespace sm;

namespace next = sm::render::next;

using render::next::DebugFlags;
using render::next::FeatureLevel;

TEST_CASE("Create next::CoreContext with debug features enabled") {
    VirtualContextTest test{0};
    auto& context = test.getContext();
    SUCCEED("Created CoreContext successfully");

    CHECK_THROWS_AS(context.setAdapter(AdapterLUID(0x1000, 0x1234)), render::RenderException);
}
