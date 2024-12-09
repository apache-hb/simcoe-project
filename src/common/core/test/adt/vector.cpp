#include "test/gtest_common.hpp"

#include "core/adt/vector.hpp"

TEST(VectorTest, Construction) {
    sm::VectorBase<int> vec;

    EXPECT_EQ(vec.size(), 0);
    EXPECT_TRUE(vec.empty());
}

TEST(VectorTest, Initialization) {
    sm::VectorBase<int> vec = { 1, 2, 3, 4 };

    EXPECT_EQ(vec.size(), 4);
    EXPECT_FALSE(vec.empty());
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[1], 2);
    EXPECT_EQ(vec[2], 3);
    EXPECT_EQ(vec[3], 4);
}

TEST(VectorTest, Move) {
    sm::VectorBase<int> vec { 1, 2, 3, 4 };

    sm::VectorBase<int> moved = std::move(vec);

    EXPECT_EQ(moved.size(), 4);
    EXPECT_FALSE(moved.empty());
    EXPECT_EQ(moved[0], 1);
    EXPECT_EQ(moved[1], 2);
    EXPECT_EQ(moved[2], 3);
    EXPECT_EQ(moved[3], 4);
}

TEST(VectorTest, Resize) {
    sm::VectorBase<int> vec;
    for (int i = 0; i < 64; ++i) {
        vec.push_back(i);
    }

    EXPECT_EQ(vec.size(), 64);
    EXPECT_FALSE(vec.empty());

    vec.resize(32);

    EXPECT_EQ(vec.size(), 32);
    EXPECT_FALSE(vec.empty());

    for (int i = 0; i < 32; ++i) {
        EXPECT_EQ(vec[i], i);
    }

    vec.resize(64);

    EXPECT_EQ(vec.size(), 64);
    EXPECT_FALSE(vec.empty());

    for (int i = 0; i < 32; ++i) {
        EXPECT_EQ(vec[i], i);
    }

    // the rest should be default initialized
    for (int i = 32; i < 64; ++i) {
        EXPECT_EQ(vec[i], int());
    }
}

TEST(VectorTest, Clear) {
    sm::VectorBase<int> vec;
    for (int i = 0; i < 64; ++i) {
        vec.push_back(i);
    }

    EXPECT_EQ(vec.size(), 64);
    EXPECT_FALSE(vec.empty());

    vec.clear();

    EXPECT_EQ(vec.size(), 0);
    EXPECT_TRUE(vec.empty());
}

TEST(VectorTest, Iteration) {
    sm::VectorBase<int> vec;
    for (int i = 0; i < 64; ++i) {
        vec.push_back(i);
    }

    int i = 0;
    for (int val : vec) {
        EXPECT_EQ(val, i);
        ++i;
    }

    EXPECT_EQ(i, 64);
}

TEST(VectorTest, NonCopyable) {
    struct NonCopyable {
        NonCopyable() = default;
        NonCopyable(const NonCopyable &) = delete;
        NonCopyable(NonCopyable &&) = default;
        NonCopyable &operator=(const NonCopyable &) = delete;
        NonCopyable &operator=(NonCopyable &&) = default;
    };

    sm::VectorBase<NonCopyable> vec;

    EXPECT_EQ(vec.size(), 0);
    EXPECT_TRUE(vec.empty());

    vec.push_back(NonCopyable());
    vec.push_back(NonCopyable());
    vec.push_back(NonCopyable());

    EXPECT_EQ(vec.size(), 3);
    EXPECT_FALSE(vec.empty());

    sm::VectorBase<NonCopyable> vec2;

    vec2.emplace_back();
    vec2.emplace_back();
    vec2.emplace_back();

    EXPECT_EQ(vec2.size(), 3);
    EXPECT_FALSE(vec2.empty());
}

TEST(VectorTest, Emplace) {
    struct Test {
        int a;
        int b;
    };

    sm::VectorBase<Test> vec;

    vec.emplace_back(1, 2);
    vec.emplace_back(3, 4);
    vec.emplace_back(5, 6);

    EXPECT_EQ(vec.size(), 3);
    EXPECT_FALSE(vec.empty());

    EXPECT_EQ(vec[0].a, 1);
    EXPECT_EQ(vec[0].b, 2);

    EXPECT_EQ(vec[1].a, 3);
    EXPECT_EQ(vec[1].b, 4);

    EXPECT_EQ(vec[2].a, 5);
    EXPECT_EQ(vec[2].b, 6);
}
