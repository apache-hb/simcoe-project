#include "test/common.hpp"

#include "config/option.hpp"

#include <sstream>

using namespace sm;
using namespace sm::config;

static void reportContextErrors(const UpdateResult& errors, bool shouldPass) {
    if (shouldPass) {
        CHECK(errors.isSuccess());
    } else {
        CHECK(errors.isFailure());
    }

    if (shouldPass) {
        for (const auto& err : errors) {
            fmt::print("Error: {}\n", err.message);
        }
    }
}

enum BitFlags {
    eFlag1 = 1,
    eFlag2 = 2,
    eFlag3 = 4,
};

enum Choices {
    eChoice1,
    eChoice2,
    eChoice3,
};

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
            CHECK(result.isSuccess());
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
            CHECK(result.isSuccess());
            CHECK(opt1.getValue() == true);
            CHECK(opt2.getValue() == 42);
            CHECK(opt3.getValue() == 3.14f);
            CHECK(opt4.getValue() == "hello");
        }
    }

    GIVEN("bitflag options") {
        Group core { name = "core", group = getCommonGroup() };

        Option<BitFlags> opt1 {
            name = "opt1",
            desc = "test description",
            group = core,
            init = BitFlags::eFlag3,
            flags = {
                val(BitFlags::eFlag1) = "flag1",
                val(BitFlags::eFlag2) = "flag2",
                val(BitFlags::eFlag3) = "flag3",
            }
        };

        Context ctx;
        ctx.addToGroup(&opt1, &core);

        std::istringstream is {
            R"(
            [core]
            opt1 = { flag1 = true, flag2 = true, flag3 = false }
            )"
        };

        auto result = ctx.updateFromConfigFile(is);

        THEN("it updates the options correctly") {
            CHECK(result.isSuccess());
            CHECK(opt1.getValue() == (BitFlags::eFlag1 | BitFlags::eFlag2));

            reportContextErrors(result, true);
        }
    }

    GIVEN("bitflag options in an inline root table") {
        Group core { name = "core", group = getCommonGroup() };

        Option<BitFlags> opt1 {
            name = "opt1",
            desc = "test description",
            group = core,
            init = BitFlags::eFlag3,
            flags = {
                val(BitFlags::eFlag1) = "flag1",
                val(BitFlags::eFlag2) = "flag2",
                val(BitFlags::eFlag3) = "flag3",
            }
        };

        Context ctx;
        ctx.addToGroup(&opt1, &core);

        std::istringstream is {
            R"(
            opt1 = { flag1 = true, flag2 = true, flag3 = false }
            )"
        };

        auto result = ctx.updateFromConfigFile(is);

        THEN("it updates the options correctly") {
            CHECK(result.isSuccess());
            CHECK(opt1.getValue() == (BitFlags::eFlag1 | BitFlags::eFlag2));

            reportContextErrors(result, true);
        }
    }

    GIVEN("enum options") {
        Option<Choices> opt1 {
            name = "opt1",
            desc = "test description",
            init = Choices::eChoice2,
            choices = {
                val(Choices::eChoice1) = "choice1",
                val(Choices::eChoice2) = "choice2",
                val(Choices::eChoice3) = "choice3",
            }
        };

        Context ctx;
        ctx.addToGroup(&opt1, &getCommonGroup());

        std::istringstream is {
            R"(
            opt1 = "choice3"
            )"
        };

        auto result = ctx.updateFromConfigFile(is);

        THEN("it updates the options correctly") {
            CHECK(result.isSuccess());
            CHECK(opt1.getValue() == Choices::eChoice3);

            reportContextErrors(result, true);
        }
    }
}
