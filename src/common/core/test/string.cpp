#include <gtest/gtest.h>

#include "core/string.hpp"

TEST(StringTest, SplitEmptyString) {
    std::string str = "";

    auto result = sm::splitAll(str, ',');
    ASSERT_TRUE(result.empty());
}

TEST(StringTest, SplitSingleEmtpy) {
    std::string str = "test";

    auto pair = sm::split(str, ',');
    ASSERT_EQ(pair.right, "");
}

TEST(StringTest, SplitSingle) {
    std::string str = "test,test";

    auto pair = sm::split(str, ',');
    ASSERT_EQ(pair.left, "test");
    ASSERT_EQ(pair.right, "test");
}

TEST(StringTest, SplitMultiple) {
    std::string str = "test,test,test";

    auto pair = sm::split(str, ',');
    ASSERT_EQ(pair.left, "test");
    ASSERT_EQ(pair.right, "test,test");
}

TEST(StringTest, SplitEnd) {
    std::string str = "test,";

    auto pair = sm::split(str, ',');
    ASSERT_EQ(pair.left, "test");
    ASSERT_EQ(pair.right, "");
}

struct SplitAllCase {
    std::string_view desc;
    std::string_view input;
    char delim;
    std::vector<std::string_view> expected;
};

SplitAllCase splitCases[] = {
    { "none", "hello", ',', { "hello" } },
    { "one", "hello,world", ',', { "hello", "world" } },
    { "leading", ",world", ',', { "", "world" } },
    { "trailing", "hello,", ',', { "hello", "" } },
    { "adjacent", "hello,,world", ',', { "hello", "", "world" } }
};

class StringSplitTest : public testing::TestWithParam<SplitAllCase> {};

INSTANTIATE_TEST_SUITE_P(
    SplitString, StringSplitTest,
    testing::ValuesIn(splitCases),
    [](const testing::TestParamInfo<SplitAllCase>& info) {
        return std::string{info.param.desc};
    });

TEST_P(StringSplitTest, SplitAll) {
    const SplitAllCase& it = GetParam();

    auto result = sm::splitAll(it.input, it.delim);
    ASSERT_EQ(result.size(), it.expected.size());
    for (size_t i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i], it.expected[i]);
    }
}

struct ReplaceAllCase {
    std::string_view desc;
    std::string_view input;
    std::string_view search;
    std::string_view replace;
    std::string expected;
};

ReplaceAllCase replaceCases[] = {
    { "none", "hello", "world", "earth", "hello" },
    { "single", "hello world", "world", "earth", "hello earth" },
    { "multiple", "hello world world", "world", "earth", "hello earth earth" },
    { "end", "hello world", "world", "earth", "hello earth" },
    { "beginning", "world hello", "world", "earth", "earth hello" }
};

class StringReplaceTest : public testing::TestWithParam<ReplaceAllCase> {};

INSTANTIATE_TEST_SUITE_P(
    ReplaceString, StringReplaceTest,
    testing::ValuesIn(replaceCases),
    [](const testing::TestParamInfo<ReplaceAllCase>& info) {
        return std::string{info.param.desc};
    });

TEST_P(StringReplaceTest, ReplaceAll) {
    const ReplaceAllCase& it = GetParam();

    std::string input{ it.input };
    sm::replaceAll(input, it.search, it.replace);
    ASSERT_EQ(input, it.expected);
}
