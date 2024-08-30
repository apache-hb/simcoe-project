#include "test/common.hpp"

#include "core/string.hpp"

TEST_CASE("string split") {
    GIVEN("a string with no delimiters") {
        std::string str = "test";

        THEN("it splits into a single element") {
            auto pair = sm::split(str, ',');
            REQUIRE(pair.right.empty());
        }
    }

    GIVEN("a string with one delimiter") {
        std::string str = "test,test";

        THEN("it splits into two elements") {
            auto pair = sm::split(str, ',');
            REQUIRE(pair.left == "test");
            REQUIRE(pair.right == "test");
        }
    }

    GIVEN("a string with multiple delimiters") {
        std::string str = "test,test,test";

        THEN("it splits into multiple elements") {
            auto pair = sm::split(str, ',');
            REQUIRE(pair.left == "test");
            REQUIRE(pair.right == "test,test");
        }
    }

    GIVEN("a string with a delimiter at the end") {
        std::string str = "test,";

        THEN("it splits into two elements") {
            auto pair = sm::split(str, ',');
            REQUIRE(pair.left == "test");
            REQUIRE(pair.right.empty());
        }
    }

    GIVEN("a string with a delimiter at the beginning") {
        std::string str = ",test";

        THEN("it splits into two elements") {
            auto pair = sm::split(str, ',');
            REQUIRE(pair.left.empty());
            REQUIRE(pair.right == "test");
        }
    }
}


TEST_CASE("Split all string") {
    struct SplitAllCase {
        std::string_view desc;
        std::string_view input;
        char delim;
        std::vector<std::string_view> expected;
    };

    SplitAllCase cases[] = {
        { "a string with no seperator", "hello", ',', { "hello" } },
        { "a string with one seperator", "hello,world", ',', { "hello", "world" } },
        { "a string with a leading seperator", ",world", ',', { "", "world" } },
        { "a string with a trailing seperator", "hello,", ',', { "hello", "" } },
        { "a string with adjacent seperators", "hello,,world", ',', { "hello", "", "world" } }
    };

    for (const SplitAllCase& it : cases) {
        GIVEN(it.desc) {
            THEN("the correct result is generated") {
                auto result = sm::splitAll(it.input, it.delim);
                CHECK(result.size() == it.expected.size());
                for (size_t i = 0; i < result.size(); i++) {
                    REQUIRE(result[i] == it.expected[i]);
                }
            }
        }
    }
}