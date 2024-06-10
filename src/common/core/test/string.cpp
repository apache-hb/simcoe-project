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
