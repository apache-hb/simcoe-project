#include "test/common.hpp"
#include "test/db_test_common.hpp"

#include "db/connection.hpp"

#include "render/base/instance.hpp"
#include "render/next/context.hpp"

using namespace sm;

namespace next = sm::render::next;

using render::next::DebugFlags;
using render::next::FeatureLevel;

TEST_CASE("Create next::CoreContext with debug features enabled") {
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
        .targetLevel = FeatureLevel::eLevel_11_0
    };

    next::CoreContext context{config};
    SUCCEED("Created CoreContext successfully");
}
