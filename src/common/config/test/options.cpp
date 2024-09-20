#include "test/common.hpp"

#include "config/config.hpp"

using namespace sm;
using namespace sm::config;

TEST_CASE("config cvar initializes correctly") {
    GIVEN("a boolean option") {
        GIVEN("default initialization") {
            sm::config::Option<bool> boolOpt {
                name = "bool-opt",
                desc = "test description"
            };

            THEN("it initializes correctly") {
                CHECK(boolOpt.name == "bool-opt");
                CHECK(boolOpt.description == "test description");
                CHECK(boolOpt.type == OptionType::eBoolean);
                CHECK(boolOpt.getInitialValue() == false);
                CHECK(boolOpt.getValue() == false);
            }
        }

        GIVEN("an option with an initial value") {
            sm::config::Option<bool> boolOpt {
                name = "bool-opt",
                desc = "test description",
                init = true
            };

            THEN("it initializes correctly") {
                CHECK(boolOpt.name == "bool-opt");
                CHECK(boolOpt.description == "test description");
                CHECK(boolOpt.type == OptionType::eBoolean);
                CHECK(boolOpt.getInitialValue() == true);
                CHECK(boolOpt.getValue() == true);
            }
        }
    }

    GIVEN("an integer option") {
        GIVEN("default initialization") {
            sm::config::Option<int> intOpt {
                name = "int-opt",
                desc = "test description"
            };

            THEN("it initializes correctly") {
                CHECK(intOpt.name == "int-opt");
                CHECK(intOpt.description == "test description");
                CHECK(intOpt.type == OptionType::eSigned);
                CHECK(intOpt.getInitialValue() == 0);
                CHECK(intOpt.getValue() == 0);
            }
        }

        GIVEN("an option with an initial value") {
            sm::config::Option<int> intOpt {
                name = "int-opt",
                desc = "test description",
                init = 42
            };

            THEN("it initializes correctly") {
                CHECK(intOpt.name == "int-opt");
                CHECK(intOpt.description == "test description");
                CHECK(intOpt.type == OptionType::eSigned);
                CHECK(intOpt.getInitialValue() == 42);
                CHECK(intOpt.getValue() == 42);
            }
        }
    }

    GIVEN("an unsigned integer option") {
        GIVEN("default initialization") {
            sm::config::Option<unsigned int> uintOpt {
                name = "uint-opt",
                desc = "test description"
            };

            THEN("it initializes correctly") {
                CHECK(uintOpt.name == "uint-opt");
                CHECK(uintOpt.description == "test description");
                CHECK(uintOpt.type == OptionType::eUnsigned);
                CHECK(uintOpt.getInitialValue() == 0);
                CHECK(uintOpt.getValue() == 0);
            }
        }

        GIVEN("an option with an initial value") {
            sm::config::Option<unsigned int> uintOpt {
                name = "uint-opt",
                desc = "test description",
                init = 42
            };

            THEN("it initializes correctly") {
                CHECK(uintOpt.name == "uint-opt");
                CHECK(uintOpt.description == "test description");
                CHECK(uintOpt.type == OptionType::eUnsigned);
                CHECK(uintOpt.getInitialValue() == 42);
                CHECK(uintOpt.getValue() == 42);
            }
        }
    }

    GIVEN("a float option") {
        GIVEN("default initialization"){
            sm::config::Option<float> floatOpt {
                name = "float-opt",
                desc = "test description"
            };

            THEN("it initializes correctly") {
                CHECK(floatOpt.name == "float-opt");
                CHECK(floatOpt.description == "test description");
                CHECK(floatOpt.type == OptionType::eReal);
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
                CHECK(floatOpt.name == "float-opt");
                CHECK(floatOpt.description == "test description");
                CHECK(floatOpt.type == OptionType::eReal);
                CHECK(floatOpt.getInitialValue() == 3.14f);
                CHECK(floatOpt.getValue() == 3.14f);
            }
        }

        GIVEN("with a range") {
            sm::config::Option<float> floatOpt {
                name = "float-opt",
                desc = "test description",
                range = Range{ 0.0f, 1.0f }
            };

            THEN("it initializes correctly") {
                CHECK(floatOpt.getRange().min == 0.0f);
                CHECK(floatOpt.getRange().max == 1.0f);
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
                CHECK(stringOpt.name == "string-opt");
                CHECK(stringOpt.description == "test description");
                CHECK(stringOpt.type == OptionType::eString);
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
                CHECK(stringOpt.name == "string-opt");
                CHECK(stringOpt.description == "test description");
                CHECK(stringOpt.type == OptionType::eString);
                CHECK(stringOpt.getInitialValue() == "initial value");
                CHECK(stringOpt.getValue() == "initial value");
            }
        }
    }
}

TEST_CASE("cvar meta flags initialize correctly") {
    GIVEN("a hidden flag") {
        sm::config::Option<bool> hiddenOpt {
            name = "hidden-opt",
            desc = "test description",
            hidden = true
        };

        THEN("it initializes correctly") {
            CHECK(hiddenOpt.isHidden());
        }
    }

    GIVEN("a readonly flag") {
        sm::config::Option<bool> readonlyOpt {
            name = "readonly-opt",
            desc = "test description",
            readonly = true
        };

        THEN("it initializes correctly") {
            CHECK(readonlyOpt.isReadOnly());
        }
    }
}

template<typename T>
void checkRangeOption(const char *testName, T min, T max) {
    GIVEN(testName) {
        sm::config::Option<T> opt {
            name = testName,
            desc = "test description"
        };

        THEN("it initializes correctly") {
            auto [min, max] = opt.getRange();
            CHECK(min == min);
            CHECK(max == max);
        }
    }
}

TEST_CASE("cvar numeric ranges are within their values bounds") {
    checkRangeOption<float>("a float option", -std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    checkRangeOption<int>("an integer option", std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    checkRangeOption<unsigned int>("an unsigned integer option", 0, std::numeric_limits<unsigned int>::max());
    checkRangeOption<unsigned short>("an unsigned short option", 0, std::numeric_limits<unsigned short>::max());
    checkRangeOption<short>("a short option", std::numeric_limits<short>::min(), std::numeric_limits<short>::max());
}

TEST_CASE("enum options") {
    GIVEN("an enum option") {
        enum class TestEnum {
            eValue1,
            eValue2,
            eValue3
        };

        sm::config::Option<TestEnum> enumOpt {
            name = "enum-opt",
            desc = "test description",
            init = TestEnum::eValue2,
            choices = {
                val(TestEnum::eValue1) = "value1",
                val(TestEnum::eValue2) = "value2",
                val(TestEnum::eValue3) = "value3"
            }
        };

        THEN("it initializes correctly") {
            CHECK(enumOpt.name == "enum-opt");
            CHECK(enumOpt.description == "test description");
            CHECK(enumOpt.getInitialValue() == TestEnum::eValue2);
            CHECK(enumOpt.getValue() == TestEnum::eValue2);
        }

        THEN("the values it contains are correct") {
            auto values = enumOpt.getCommonOptions();
            CHECK(values.size() == 3);
            CHECK(values[0].name == "value1");
            CHECK(values[0].value == std::to_underlying(TestEnum::eValue1));
            CHECK(values[1].name == "value2");
            CHECK(values[1].value == std::to_underlying(TestEnum::eValue2));
            CHECK(values[2].name == "value3");
            CHECK(values[2].value == std::to_underlying(TestEnum::eValue3));
        }
    }
}