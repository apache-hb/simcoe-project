#include "test_common.hpp"

#include "config/cvar.hpp"

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
                CHECK(boolOpt.getName() == "bool-opt");
                CHECK(boolOpt.getDescription() == "test description");
                CHECK(boolOpt.getType() == OptionType::eBoolean);
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
                CHECK(boolOpt.getName() == "bool-opt");
                CHECK(boolOpt.getDescription() == "test description");
                CHECK(boolOpt.getType() == OptionType::eBoolean);
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
                CHECK(intOpt.getName() == "int-opt");
                CHECK(intOpt.getDescription() == "test description");
                CHECK(intOpt.getType() == OptionType::eSigned);
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
                CHECK(intOpt.getName() == "int-opt");
                CHECK(intOpt.getDescription() == "test description");
                CHECK(intOpt.getType() == OptionType::eSigned);
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
                CHECK(uintOpt.getName() == "uint-opt");
                CHECK(uintOpt.getDescription() == "test description");
                CHECK(uintOpt.getType() == OptionType::eUnsigned);
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
                CHECK(uintOpt.getName() == "uint-opt");
                CHECK(uintOpt.getDescription() == "test description");
                CHECK(uintOpt.getType() == OptionType::eUnsigned);
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

TEST_CASE("cvar numeric ranges are within their values bounds") {
    GIVEN("a float option") {
        sm::config::Option<float> floatOpt {
            name = "float-opt",
            desc = "test description"
        };

        THEN("it initializes correctly") {
            auto [min, max] = floatOpt.getRange();
            CHECK(min == std::numeric_limits<float>::min());
            CHECK(max == std::numeric_limits<float>::max());
        }
    }

    GIVEN("an integer option") {
        sm::config::Option<int> intOpt {
            name = "int-opt",
            desc = "test description"
        };

        THEN("it initializes correctly") {
            auto [min, max] = intOpt.getRange();
            CHECK(min == std::numeric_limits<int>::min());
            CHECK(max == std::numeric_limits<int>::max());
        }
    }

    GIVEN("an unsigned integer option") {
        sm::config::Option<unsigned int> uintOpt {
            name = "uint-opt",
            desc = "test description"
        };

        THEN("it initializes correctly") {
            auto [min, max] = uintOpt.getRange();
            CHECK(min == std::numeric_limits<unsigned int>::min());
            CHECK(max == std::numeric_limits<unsigned int>::max());
        }
    }
}