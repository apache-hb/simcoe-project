#include "render_test_common.hpp"

using namespace sm;

namespace next = sm::render::next;

using render::next::FeatureLevel;
using render::AdapterLUID;

TEST_CASE("Create next::CoreContext") {
    VirtualContextTest test{0, false};
    auto& context = test.getContext();
    SUCCEED("Created CoreContext successfully");

    CHECK_THROWS_AS(context.setAdapter(AdapterLUID(0x1000, 0x1234)), render::RenderException);
}
