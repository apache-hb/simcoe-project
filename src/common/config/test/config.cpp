#include "test_common.hpp"

#include "config/option.hpp"

using namespace sm;
using namespace sm::config;

TEST_CASE("config files update options") {
    GIVEN("a simple config") {
        Group core { name = "core", group = getCommonGroup() };
        Group graphics { name = "graphics", group = getCommonGroup() };

        Option<bool> opt1 { name = "opt1", desc = "test description", group = core };
        Option<int> opt2 { name = "opt2", desc = "test description", group = core };
        Option<float> opt3 { name = "opt3", desc = "test description", group = graphics };

        Context ctx;
        ctx.addToGroup(&opt1, &core);
        ctx.addToGroup(&opt2, &core);
        ctx.addToGroup(&opt3, &graphics);

        std::istringstream is {
            R"(
            [core]
            opt1 = true
            opt2 = 42

            [graphics]
            opt3 = 3.14
            )"
        };

        auto result = ctx.updateFromConfigFile(is);

        THEN("it updates the options correctly") {
            CHECK(result.errors.empty());
            CHECK(opt1.getValue() == true);
            CHECK(opt2.getValue() == 42);
            CHECK(opt3.getValue() == 3.14f);
        }
    }

    GIVEN("nested groups") {
        Group core { name = "core", group = getCommonGroup() };
        Group graphics { name = "graphics", group = getCommonGroup() };
        Group nested { name = "nested", group = core };

        Option<bool> opt1 { name = "opt1", desc = "test description", group = core };
        Option<int> opt2 { name = "opt2", desc = "test description", group = core };
        Option<float> opt3 { name = "opt3", desc = "test description", group = graphics };
        Option<std::string> opt4 { name = "opt4", desc = "test description", group = nested };

        Context ctx;
        ctx.addToGroup(&opt1, &core);
        ctx.addToGroup(&opt2, &core);
        ctx.addToGroup(&opt3, &graphics);
        ctx.addToGroup(&opt4, &nested);

        std::istringstream is {
            R"(
            [core]
            opt1 = true
            opt2 = 42

            [graphics]
            opt3 = 3.14

            [core.nested]
            opt4 = "hello"
            )"
        };

        auto result = ctx.updateFromConfigFile(is);

        THEN("it updates the options correctly") {
            CHECK(result.errors.empty());
            CHECK(opt1.getValue() == true);
            CHECK(opt2.getValue() == 42);
            CHECK(opt3.getValue() == 3.14f);
            CHECK(opt4.getValue() == "hello");
        }
    }
}
