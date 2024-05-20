#include "test_common.hpp"

#include "config/cvar.hpp"

using namespace sm;
using namespace sm::config;

TEST_CASE("config cvar initializes correctly") {
    GIVEN("a boolean option") {
        sm::config::Option<bool> boolOpt {
            name = "bool-opt",
            desc = "test description"
        };

        THEN("it initializes correctly") {
            CHECK(boolOpt.getName() == "bool-opt");
            CHECK(boolOpt.getDescription() == "test description");
            CHECK(boolOpt.getType() == OptionType::eBoolean);
            CHECK(boolOpt.getInitialValue() == false);
            CHECK(boolOpt.getValue() == false);
        }
    }

    GIVEN("an integer option") {
        sm::config::Option<int> intOpt {
            name = "int-opt",
            desc = "test description"
        };

        THEN("it initializes correctly") {
            CHECK(intOpt.getName() == "int-opt");
            CHECK(intOpt.getDescription() == "test description");
            CHECK(intOpt.getType() == OptionType::eInteger);
            CHECK(intOpt.getInitialValue() == 0);
            CHECK(intOpt.getValue() == 0);
        }
    }

    GIVEN("a float option") {
        GIVEN("default initialization"){
            sm::config::Option<float> floatOpt {
                name = "float-opt",
                desc = "test description"
            };

            THEN("it initializes correctly") {
                CHECK(floatOpt.getName() == "float-opt");
                CHECK(floatOpt.getDescription() == "test description");
                CHECK(floatOpt.getType() == OptionType::eReal);
                CHECK(floatOpt.getInitialValue() == 0.0f);
                CHECK(floatOpt.getValue() == 0.0f);
            }
        }

        GIVEN("an option with an initial value") {
            sm::config::Option<float> floatOpt {
                name = "float-opt",
                desc = "test description",
                init = 3.14f
            };

            THEN("it initializes correctly") {
                CHECK(floatOpt.getName() == "float-opt");
                CHECK(floatOpt.getDescription() == "test description");
                CHECK(floatOpt.getType() == OptionType::eReal);
                CHECK(floatOpt.getInitialValue() == 3.14f);
                CHECK(floatOpt.getValue() == 3.14f);
            }
        }
    }

    GIVEN("a string option") {
        GIVEN("default initialization") {
            sm::config::Option<std::string> stringOpt {
                name = "string-opt",
                desc = "test description"
            };

            THEN("it initializes correctly") {
                CHECK(stringOpt.getName() == "string-opt");
                CHECK(stringOpt.getDescription() == "test description");
                CHECK(stringOpt.getType() == OptionType::eString);
                CHECK(stringOpt.getInitialValue() == "");
                CHECK(stringOpt.getValue() == "");
            }
        }

        GIVEN("a string option with an initial value") {
            sm::config::Option<std::string> stringOpt {
                name = "string-opt",
                desc = "test description",
                init = "initial value"
            };

            THEN("it initializes correctly") {
                CHECK(stringOpt.getName() == "string-opt");
                CHECK(stringOpt.getDescription() == "test description");
                CHECK(stringOpt.getType() == OptionType::eString);
                CHECK(stringOpt.getInitialValue() == "initial value");
                CHECK(stringOpt.getValue() == "initial value");
            }
        }
    }
}
