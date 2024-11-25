#include <gtest/gtest.h>

#include "core/adt/zstring_view.hpp"

using namespace sm::literals;

TEST(ZStringTest, DefaultInitialization) {
    sm::ZStringView view;

    ASSERT_EQ(view.size(), 0);
    ASSERT_TRUE(view.empty());
}

TEST(ZStringTest, StringConstruction) {
    sm::ZStringView view = "test";

    ASSERT_EQ(view.size(), 4);
    ASSERT_FALSE(view.empty());
    bool eq = view == "test"_zsv;
    ASSERT_TRUE(eq);
}

TEST(ZStringTest, ArrayLiteralInit) {
    sm::ZStringView view = "test";

    ASSERT_EQ(view.size(), 4);
    ASSERT_FALSE(view.empty());
    ASSERT_EQ(view, "test");
}

TEST(ZStringTest, EmptyString) {
    sm::ZStringView view = "";

    ASSERT_EQ(view.size(), 0);
    ASSERT_TRUE(view.empty());
    ASSERT_EQ(view, "");
}

TEST(ZStringTest, Comparison) {
    sm::ZStringView view = "test";

    ASSERT_TRUE(view == "test");
    ASSERT_TRUE(view != "test2");
    ASSERT_TRUE(view < "test2");
    ASSERT_TRUE(view <= "test2");
    ASSERT_TRUE(view <= "test");
    ASSERT_TRUE(view > "tes1");
    ASSERT_TRUE(view >= "tes");
    ASSERT_TRUE(view >= "test");
}

TEST(ZStringTest, Operations) {
    ASSERT_TRUE("test"_zsv.startsWith("te"));
    ASSERT_TRUE("test"_zsv.endsWith("st"));
}

TEST(ZStringTest, Indexing) {
    sm::ZStringView view = "test";

    ASSERT_EQ(view.length(), 4);
    ASSERT_EQ(view[0], 't');
    ASSERT_EQ(view[1], 'e');
    ASSERT_EQ(view[2], 's');
    ASSERT_EQ(view[3], 't');
}
