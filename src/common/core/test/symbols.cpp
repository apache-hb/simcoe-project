#include "test/common.hpp"

#include "core/macros.hpp"

namespace com::apache::simcoe {
    class Thing {

    };
}

struct SomeType;

TEST_CASE("symbol names") {

    GIVEN("a type") {
        THEN("it is formatted properly") {
            REQUIRE(sm::getTypeName<SomeType>() == "SomeType");
        }
    }

    GIVEN("a type in a namespace") {
        THEN("it is formatted properly") {
            REQUIRE(sm::getTypeName<com::apache::simcoe::Thing>() == "com.apache.simcoe.Thing");
        }
    }
}
