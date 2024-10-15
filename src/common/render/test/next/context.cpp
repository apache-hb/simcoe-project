#include "test/common.hpp"
#include "test/db_test_common.hpp"

#include "db/connection.hpp"

#include "render/base/instance.hpp"
#include "render/next/context.hpp"

using namespace sm;

namespace next = sm::render::next;

using render::next::FeatureLevel;
using render::AdapterLUID;

TEST_CASE("Create next::CoreContext") {
    next::VirtualSwapChainFactory swapChainFactory { };

    next::ContextConfig config {
        .targetLevel = FeatureLevel::eLevel_11_0,
        .swapChainFactory = &swapChainFactory,
        .swapChainInfo = {
            .format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .size = { 1920, 1080 },
            .length = 2,
            .clearColour = { 0.0f, 0.0f, 0.0f, 1.0f },
        },
    };

    next::CoreContext context{config};
    SUCCEED("Created CoreContext successfully");

    CHECK_THROWS_AS(context.setAdapter(AdapterLUID(0x1000, 0x1234)), render::RenderException);
}
