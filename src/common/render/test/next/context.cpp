#include "test/common.hpp"
#include "test/db_test_common.hpp"

#include "db/connection.hpp"

#include "render/base/instance.hpp"
#include "render/next/context.hpp"

using namespace sm;

namespace next = sm::render::next;

using render::next::FeatureLevel;

TEST_CASE("Create next::CoreContext") {
    next::ContextConfig config {
        .targetLevel = FeatureLevel::eLevel_11_0
    };

    next::CoreContext context{config};
    SUCCEED("Created CoreContext successfully");
}
