#include "test/common.hpp"

#include "config/option.hpp"

using namespace sm;
using namespace sm::config;

static UpdateResult testCommandLine(config::Context& ctx, std::initializer_list<const char*> args) {
    int argc = args.size();
    const char* const* argv = args.begin();
    return ctx.updateFromCommandLine(argc, argv);
}

static void reportContextErrors(const UpdateResult& err, bool shouldPass) {
    if (shouldPass) {
        CHECK(err.errors.empty());
    } else {
        CHECK(!err.errors.empty());
    }

    if (shouldPass) {
        for (const auto& err : err.errors) {
            fmt::print("Error: {}\n", err.message);
        }
    }
}

static void testVariants(config::Context& ctx, const char *optName, const char *optValue, bool shouldPass, auto cb) {
    GIVEN("a unix flag") {
        std::string opt = fmt::format("--{}", optName);
        auto err = testCommandLine(ctx, { "test", opt.c_str(), optValue });
        cb();

        reportContextErrors(err, shouldPass);
    }

    GIVEN("a unix flag assigned value") {
        std::string opt = fmt::format("--{}={}", optName, optValue);
        auto err = testCommandLine(ctx, { "test", opt.c_str() });
        cb();

        reportContextErrors(err, shouldPass);
    }

    GIVEN("a dos flag") {
        std::string opt = fmt::format("/{}", optName);
        auto err = testCommandLine(ctx, { "test", opt.c_str(), optValue });
        cb();

        reportContextErrors(err, shouldPass);
    }

    GIVEN("a dos flag assigned value") {
        std::string opt = fmt::format("/{}:{}", optName, optValue);
        auto err = testCommandLine(ctx, { "test", opt.c_str() });
        cb();

        reportContextErrors(err, shouldPass);
    }
}

TEST_CASE("parsing a command line") {
    GIVEN("no options") {
        config::Context ctx;
        testCommandLine(ctx, { "test" });
    }

    GIVEN("a boolean option") {
        config::Context ctx;
        Option<bool> boolOpt {
            name = "bool-opt",
            desc = "test description"
        };
        ctx.addToGroup(&boolOpt, &config::getCommonGroup());

        WHEN("the option is not set") {
            testCommandLine(ctx, { "test" });

            THEN("the option is false") {
                CHECK(boolOpt.getValue() == false);
            }
        }

        WHEN("the option is set") {
            testCommandLine(ctx, { "test", "--bool-opt" });

            THEN("the option is true") {
                CHECK(boolOpt.getValue() == true);
                CHECK(boolOpt.isSet());
            }
        }

        WHEN("using dos flags") {
            testCommandLine(ctx, { "test", "/bool-opt" });

            THEN("the option is true") {
                CHECK(boolOpt.getValue() == true);
                CHECK(boolOpt.isSet());
            }
        }

        WHEN("the option is set to false") {
            testCommandLine(ctx, { "test", "--bool-opt=false" });

            THEN("the option is false") {
                CHECK(boolOpt.getValue() == false);
                CHECK(boolOpt.isSet());
            }
        }

        WHEN("the option is set to false using dos flags") {
            testCommandLine(ctx, { "test", "/bool-opt:false" });

            THEN("the option is false") {
                CHECK(boolOpt.getValue() == false);
                CHECK(boolOpt.isSet());
            }
        }

        WHEN("using a seperate flag value") {
            testCommandLine(ctx, { "test", "--bool-opt", "false" });

            THEN("the option is false") {
                CHECK(boolOpt.getValue() == false);
                CHECK(boolOpt.isSet());
            }
        }

        WHEN("using a seperate flag value with dos flags") {
            testCommandLine(ctx, { "test", "/bool-opt", "false" });

            THEN("the option is false") {
                CHECK(boolOpt.getValue() == false);
                CHECK(boolOpt.isSet());
            }
        }
    }

    GIVEN("a string option") {
        config::Context ctx;
        Option<std::string> strOpt {
            name = "str-opt",
            desc = "test description"
        };
        ctx.addToGroup(&strOpt, &config::getCommonGroup());

        WHEN("the option is not set") {
            testCommandLine(ctx, { "test" });

            THEN("the option is empty") {
                CHECK(strOpt.getValue().empty());
            }
        }

        WHEN("the option is set") {
            testVariants(ctx, "str-opt", "value", true, [&] {
                THEN("the option is set") {
                    CHECK(strOpt.getValue() == "value");
                    CHECK(strOpt.isSet());
                }
            });
        }

        WHEN("the option contains spaces") {
            testVariants(ctx, "str-opt", "value with spaces", true, [&] {
                THEN("the option is set") {
                    CHECK(strOpt.getValue() == "value with spaces");
                    CHECK(strOpt.isSet());
                }
            });
        }

        WHEN("the option is set to an empty string") {
            testVariants(ctx, "str-opt", "", true, [&] {
                THEN("the option is set") {
                    CHECK(strOpt.getValue().empty());
                    CHECK(strOpt.isSet());
                }
            });
        }
    }

    GIVEN("a float option") {
        config::Context ctx;
        Option<float> floatOpt {
            name = "float-opt",
            desc = "test description"
        };
        ctx.addToGroup(&floatOpt, &config::getCommonGroup());

        WHEN("the option is not set") {
            testCommandLine(ctx, { "test" });

            THEN("the option is 0.0") {
                CHECK(floatOpt.getValue() == 0.0f);
            }
        }

        WHEN("the option is set") {
            testVariants(ctx, "float-opt", "3.14", true, [&] {
                THEN("the option is set") {
                    CHECK(floatOpt.getValue() == 3.14f);
                    CHECK(floatOpt.isSet());
                }
            });
        }

        WHEN("the option is out of range") {
            testVariants(ctx, "float-opt", "1e1000", false, [&] {
                THEN("the option is not set") {
                    CHECK(floatOpt.getValue() == 0.0f);
                    CHECK(!floatOpt.isSet());
                }
            });
        }

        WHEN("the option overflows") {
            testVariants(ctx, "float-opt", "3.4028235e38", false, [&] {
                THEN("the option is set") {
                    CHECK(floatOpt.getValue() == 0.0f);
                    CHECK(!floatOpt.isSet());
                }
            });
        }

        WHEN("the option underflows") {
            testVariants(ctx, "float-opt", "-3.4028235e38", false, [&] {
                THEN("the option is set") {
                    CHECK(floatOpt.getValue() == 0.0f);
                    CHECK(!floatOpt.isSet());
                }
            });
        }

        WHEN("the option is negative zero") {
            testVariants(ctx, "float-opt", "-0.0", true, [&] {
                THEN("the option is set") {
                    CHECK(floatOpt.getValue() == -0.0f);
                    CHECK(floatOpt.isSet());
                }
            });
        }

        WHEN("the option is positive zero") {
            testVariants(ctx, "float-opt", "0.0", true, [&] {
                THEN("the option is set") {
                    CHECK(floatOpt.getValue() == 0.0f);
                    CHECK(floatOpt.isSet());
                }
            });
        }
    }

    GIVEN("an unsigned type option") {
        auto checkContext = [](const char *caseName, auto cb) {
            config::Context context;
            Option<unsigned> option {
                name = "uint-opt",
                desc = "test description"
            };

            context.addToGroup(&option, &config::getCommonGroup());

            GIVEN(caseName) {
                cb(context, option);
            }
        };

        checkContext("the option is not set", [](auto& ctx, auto& opt) {
            testCommandLine(ctx, { "test" });

            THEN("the option is 0") {
                CHECK(opt.getValue() == 0);
            }
        });

        checkContext("the option is set", [](auto& ctx, auto& opt) {
            testVariants(ctx, "uint-opt", "42", true, [&] {
                THEN("the option is set") {
                    CHECK(opt.getValue() == 42);
                    CHECK(opt.isSet());
                }
            });
        });

        checkContext("the value is out of range", [](auto& ctx, auto& opt) {
            testVariants(ctx, "uint-opt", "1e1000", false, [&] {
                THEN("the option is not set") {
                    CHECK(opt.getValue() == 0);
                    CHECK(!opt.isSet());
                }
            });
        });

        checkContext("the value overflows in base10", [](auto& ctx, auto& opt) {
            testVariants(ctx, "uint-opt", "4294967296", false, [&] {
                THEN("the option is not set") {
                    CHECK(opt.getValue() == 0);
                    CHECK(!opt.isSet());
                }
            });
        });

        checkContext("the value underflows in base10", [](auto& ctx, auto& opt) {
            testVariants(ctx, "uint-opt", "-1", false, [&] {
                THEN("the option is not set") {
                    CHECK(opt.getValue() == 0);
                    CHECK(!opt.isSet());
                }
            });
        });

        checkContext("the value is negative", [](auto& ctx, auto& opt) {
            testVariants(ctx, "uint-opt", "-42", false, [&] {
                THEN("the option is not set") {
                    CHECK(opt.getValue() == 0);
                    CHECK(!opt.isSet());
                }
            });
        });

        checkContext("the value is zero", [](auto& ctx, auto& opt) {
            testVariants(ctx, "uint-opt", "0", true, [&] {
                THEN("the option is set") {
                    CHECK(opt.getValue() == 0);
                    CHECK(opt.isSet());
                }
            });
        });

        checkContext("the value is the maximum", [](auto& ctx, auto& opt) {
            testVariants(ctx, "uint-opt", "4294967295", true, [&] {
                THEN("the option is set") {
                    CHECK(opt.getValue() == 4294967295);
                    CHECK(opt.isSet());
                }
            });
        });

        checkContext("the value is negative zero", [](auto& ctx, auto& opt) {
            testVariants(ctx, "uint-opt", "-0", false, [&] {
                THEN("the option is not set") {
                    CHECK(opt.getValue() == 0);
                    CHECK(!opt.isSet());
                }
            });
        });

        checkContext("the value is positive zero", [](auto& ctx, auto& opt) {
            testVariants(ctx, "uint-opt", "0", true, [&] {
                THEN("the option is set") {
                    CHECK(opt.getValue() == 0);
                    CHECK(opt.isSet());
                }
            });
        });
    }

    GIVEN("a signed type option") {
        auto checkContext = [](const char *caseName, auto cb) {
            config::Context context;
            Option<int> option {
                name = "int-opt",
                desc = "test description"
            };

            context.addToGroup(&option, &config::getCommonGroup());

            GIVEN(caseName) {
                cb(context, option);
            }
        };

        checkContext("the option is not set", [](auto& ctx, auto& opt) {
            testCommandLine(ctx, { "test" });

            THEN("the option is 0") {
                CHECK(opt.getValue() == 0);
            }
        });

        checkContext("the option is set", [](auto& ctx, auto& opt) {
            testCommandLine(ctx, { "test", "--int-opt", "42" });

            THEN("the option is set") {
                CHECK(opt.getValue() == 42);
                CHECK(opt.isSet());
            }
        });

        checkContext("using dos flags", [](auto& ctx, auto& opt) {
            testCommandLine(ctx, { "test", "/int-opt:42" });

            THEN("the option is set") {
                CHECK(opt.getValue() == 42);
                CHECK(opt.isSet());
            }
        });

        checkContext("the value is out of range", [](auto& ctx, auto& opt) {
            testCommandLine(ctx, { "test", "--int-opt", "1e1000" });

            THEN("the option is not set") {
                CHECK(opt.getValue() == 0);
                CHECK(!opt.isSet());
            }
        });

        checkContext("the value is out of range in base 10", [](auto& ctx, auto& opt) {
            testCommandLine(ctx, { "test", "--int-opt", "2147483648" });

            THEN("the option is not set") {
                CHECK(opt.getValue() == 0);
                CHECK(!opt.isSet());
            }
        });

        checkContext("value underflows in base 10", [](auto& ctx, auto& opt) {
            testCommandLine(ctx, { "test", "--int-opt", "-2147483649" });

            THEN("the option is not set") {
                CHECK(opt.getValue() == 0);
                CHECK(!opt.isSet());
            }
        });

        checkContext("the value is negative", [](auto& ctx, auto& opt) {
            testCommandLine(ctx, { "test", "--int-opt", "-42" });

            THEN("the option is set") {
                CHECK(opt.getValue() == -42);
                CHECK(opt.isSet());
            }
        });

        checkContext("the value is zero", [](auto& ctx, auto& opt) {
            testCommandLine(ctx, { "test", "--int-opt", "0" });

            THEN("the option is set") {
                CHECK(opt.getValue() == 0);
                CHECK(opt.isSet());
            }
        });

        checkContext("the value is the minimum", [](auto& ctx, auto& opt) {
            testCommandLine(ctx, { "test", "--int-opt", "-2147483648" });

            THEN("the option is set") {
                CHECK(opt.getValue() == -2147483648);
                CHECK(opt.isSet());
            }
        });
    }
}

enum TestEnum {
    eFirst, eSecond, eThird
};

TEST_CASE("command line and enum options") {

    GIVEN("a valid value") {
        Option<TestEnum> opt1 {
            name = "opt1",
            desc = "test description",
            options = {
                val(TestEnum::eFirst) = "first",
                val(TestEnum::eSecond) = "second",
                val(TestEnum::eThird) = "third"
            },
            init = TestEnum::eFirst
        };

        Context ctx;
        ctx.addToGroup(&opt1, &getCommonGroup());

        reportContextErrors(testCommandLine(ctx, { "test", "--opt1=second" }), true);

        THEN("it updates the option correctly") {
            CHECK(opt1.getValue() == TestEnum::eSecond);
            CHECK(opt1.isSet());
        }
    }

    GIVEN("an invalid value") {
        Option<TestEnum> opt1 {
            name = "opt1",
            desc = "test description",
            options = {
                val(TestEnum::eFirst) = "first",
                val(TestEnum::eSecond) = "second",
                val(TestEnum::eThird) = "third"
            }
        };

        Context ctx;

        ctx.addToGroup(&opt1, &getCommonGroup());

        reportContextErrors(testCommandLine(ctx, { "test", "--opt1=invalid" }), false);

        THEN("it does not update the option") {
            CHECK(opt1.getValue() == TestEnum::eFirst);
            CHECK(!opt1.isSet());
        }
    }

    enum TestFlags {
        eEmpty = 0,
        eFlag1 = 1 << 0,
        eFlag2 = 1 << 1,
        eFlag3 = 1 << 2
    };

    GIVEN("a bitflag option") {
        Option<TestFlags> opt1 {
            name = "opt1",
            desc = "test description",
            flags = {
                val(TestFlags::eFlag1) = "first",
                val(TestFlags::eFlag2) = "second",
                val(TestFlags::eFlag3) = "third"
            },
            init = TestFlags::eEmpty
        };

        Context ctx;
        ctx.addToGroup(&opt1, &getCommonGroup());

        reportContextErrors(testCommandLine(ctx, { "test", "--opt1=second,third" }), true);

        THEN("it updates the option correctly") {
            CHECK(opt1.getValue() == (TestFlags::eFlag2 | TestFlags::eFlag3));
            CHECK(opt1.isSet());
        }
    }

    GIVEN("negating bitflag option") {
        Option<TestFlags> opt1 {
            name = "opt1",
            desc = "test description",
            flags = {
                val(TestFlags::eFlag1) = "first",
                val(TestFlags::eFlag2) = "second",
                val(TestFlags::eFlag3) = "third"
            },
            init = TestFlags::eFlag2
        };

        Context ctx;
        ctx.addToGroup(&opt1, &getCommonGroup());

        reportContextErrors(testCommandLine(ctx, { "test", "--opt1=third,-second" }), true);

        THEN("it updates the option correctly") {
            CHECK(opt1.getValue() == TestFlags::eFlag3);
            CHECK(opt1.isSet());
        }
    }

    GIVEN("multiple bitflag usages") {
        Option<TestFlags> opt1 {
            name = "opt1",
            desc = "test description",
            flags = {
                val(TestFlags::eFlag1) = "first",
                val(TestFlags::eFlag2) = "second",
                val(TestFlags::eFlag3) = "third"
            },
            init = TestFlags::eFlag2
        };

        Context ctx;
        ctx.addToGroup(&opt1, &getCommonGroup());

        // TODO: is this the behaviour we want?
        reportContextErrors(testCommandLine(ctx, { "test", "--opt1=third", "--opt1=-second" }), true);

        THEN("it updates the option correctly") {
            CHECK(opt1.getValue() == TestFlags::eFlag3);
            CHECK(opt1.isSet());
        }
    }
}
