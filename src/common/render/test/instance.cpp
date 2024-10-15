#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include "test/common.hpp"

#include "render/render.hpp"

#include <set>

using namespace sm;

static constexpr render::InstanceConfig kConfigOptions[] = {
    // default case
    render::InstanceConfig { },
    render::InstanceConfig {
        .preference = render::AdapterPreference::eHighPerformance,
    },
    render::InstanceConfig {
        .preference = render::AdapterPreference::eMinimumPower,
    },
    render::InstanceConfig {
        .flags = render::DebugFlags::eDeviceDebugLayer
               | render::DebugFlags::eFactoryDebug
               | render::DebugFlags::eDeviceRemovedInfo
               | render::DebugFlags::eInfoQueue
               | render::DebugFlags::eAutoName,
    }
};

TEST_CASE("Setup rendering instance") {
    auto config = GENERATE(from_range(kConfigOptions));

    GIVEN("An instance config") {
        render::Instance instance{config};

        WHEN("The instance is created") {
            CHECK(instance.factory() != nullptr);
            CHECK(instance.flags() == config.flags);

            THEN("It has a warp adapter") {
                auto& warp = instance.getWarpAdapter();
                CHECK(warp.isValid());
                CHECK(warp.flags().test(render::AdapterFlag::eSoftware));
            }

            THEN("It has a default adapter") {
                CHECK(instance.getDefaultAdapter().isValid());
            }

            THEN("It has a viable adapter") {
                CHECK(instance.hasViableAdapter());
            }

            THEN("The adapter list is valid") {
                auto adapters = instance.adapters();
                CHECK(adapters.size() > 0); // must always be > 0 because of the warp adapter

                std::set<render::AdapterLUID> luids;
                for (const auto& adapter : adapters) {
                    CHECK(adapter.isValid());
                    CHECK(!luids.contains(adapter.luid()));
                }
            }
        }
    }
}
