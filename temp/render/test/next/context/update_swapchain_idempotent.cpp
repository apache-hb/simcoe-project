#include "render_test_common.hpp"

using namespace sm;

namespace next = sm::render::next;

using render::next::FeatureLevel;
using render::AdapterLUID;

TEST_CASE("Create next::CoreContext") {
    VirtualContextTest test{5};
    auto& context = test.getContext();
    SUCCEED("Created CoreContext successfully");

    context.setAdapter(context.getAdapter());
    context.setAdapter(context.getAdapter());
    context.setAdapter(context.getAdapter());

    test.run();
}
