#include "test/common.hpp"

#include "core/adt/zstring_view.hpp"

using namespace sm::literals;

TEST_CASE("zstring view operations") {
    GIVEN("default initialization") {
        sm::ZStringView view;

        THEN("it initializes correctly") {
            CHECK(view.size() == 0);
            CHECK(view.empty());
        }
    }

    GIVEN("string construction") {
        sm::ZStringView view = "test";

        THEN("it initializes correctly") {
            CHECK(view.size() == 4);
            CHECK_FALSE(view.empty());
            bool eq = view == "test"_zsv;
            CHECK(eq);
        }
    }

    GIVEN("array literal construction") {
        sm::ZStringView view = "test";

        THEN("it initializes correctly") {
            CHECK(view.size() == 4);
            CHECK_FALSE(view.empty());
            CHECK(view == "test");
        }
    }

    GIVEN("an empty string") {
        sm::ZStringView view = "";

        THEN("it initializes correctly") {
            CHECK(view.size() == 0);
            CHECK(view.empty());
            CHECK(view == "");
        }
    }

    GIVEN("comparison with a string") {
        sm::ZStringView view = "test";

        THEN("it compares correctly") {
            CHECK(view == "test");
            CHECK(view != "test2");
            CHECK(view < "test2");
            CHECK(view <= "test2");
            CHECK(view <= "test");
            CHECK(view > "tes1");
            CHECK(view >= "tes");
            CHECK(view >= "test");
        }
    }

    CHECK(sm::isNullTerminated("test"));

    CHECK("test"_zsv.startsWith("te"));
    CHECK("test"_zsv.endsWith("st"));

    GIVEN("a string") {
        sm::ZStringView view = "test";

        THEN("its values are correct") {
            CHECK(view.length() == 4);
            CHECK(view[0] == 't');
            CHECK(view[1] == 'e');
            CHECK(view[2] == 's');
            CHECK(view[3] == 't');
        }
    }
}
