#include <gtest/gtest.h>

#include "core/adt/small_vector.hpp"

TEST(VectorTest, DefaultInitialization) {
    sm::SmallVector<int, 4> vec;

    ASSERT_EQ(vec.size(), 0);
    ASSERT_TRUE(vec.isEmpty());
}

TEST(VectorTest, Append) {
    sm::SmallVector<int, 4> vec;

    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    ASSERT_EQ(vec.size(), 3);
    ASSERT_FALSE(vec.isEmpty());
    ASSERT_EQ(vec[0], 1);
    ASSERT_EQ(vec[1], 2);
    ASSERT_EQ(vec[2], 3);
}

TEST(VectorTest, InitialValues) {
    sm::SmallVector<int, 4> vec = { 1, 2, 3, 4 };

    ASSERT_EQ(vec.size(), 4);
    ASSERT_FALSE(vec.isEmpty());
    ASSERT_EQ(vec[0], 1);
    ASSERT_EQ(vec[1], 2);
    ASSERT_EQ(vec[2], 3);
    ASSERT_EQ(vec[3], 4);
}

TEST(VectorTest, Move) {
    sm::SmallVector<int, 4> vec { 1, 2, 3, 4 };

    sm::SmallVector<int, 4> moved = std::move(vec);

    ASSERT_EQ(moved.size(), 4);
    ASSERT_FALSE(moved.isEmpty());
    ASSERT_EQ(moved[0], 1);
    ASSERT_EQ(moved[1], 2);
    ASSERT_EQ(moved[2], 3);
    ASSERT_EQ(moved[3], 4);
}

TEST(VectorTest, Copy) {
    sm::SmallVector<int, 4> vec { 1, 2, 3, 4 };

    sm::SmallVector<int, 4> copy = vec.clone();

    ASSERT_EQ(copy.size(), 4);
    ASSERT_FALSE(copy.isEmpty());
    ASSERT_EQ(copy[0], 1);
    ASSERT_EQ(copy[1], 2);
    ASSERT_EQ(copy[2], 3);
    ASSERT_EQ(copy[3], 4);
}

TEST(VectorTest, TryEmplaceBack) {
    sm::SmallVector<int, 4> vec;

    ASSERT_TRUE(vec.tryEmplaceBack(1));
    ASSERT_TRUE(vec.tryEmplaceBack(2));
    ASSERT_TRUE(vec.tryEmplaceBack(3));
    ASSERT_TRUE(vec.tryEmplaceBack(4));
    ASSERT_FALSE(vec.tryEmplaceBack(5));

    ASSERT_EQ(vec.size(), 4);
    ASSERT_FALSE(vec.isEmpty());
    ASSERT_EQ(vec[0], 1);
    ASSERT_EQ(vec[1], 2);
    ASSERT_EQ(vec[2], 3);
    ASSERT_EQ(vec[3], 4);
}

TEST(VectorTest, PopBackEmpty) {
    sm::SmallVector<int, 4> vec;

    ASSERT_FALSE(vec.tryPopBack());
}

TEST(VectorTest, ResizeInvalid) {
    sm::SmallVector<int, 4> vec;

    ASSERT_TRUE(vec.tryResize(0));
    ASSERT_TRUE(vec.tryResize(4));
    ASSERT_FALSE(vec.tryResize(5));
    ASSERT_FALSE(vec.tryResize(-1));
}

TEST(VectorTest, ConstVector) {
    const sm::SmallVector<int, 4> vec { 1, 2, 3, 4 };

    ASSERT_EQ(vec.size(), 4);
    ASSERT_FALSE(vec.isEmpty());
    ASSERT_EQ(vec[0], 1);
    ASSERT_EQ(vec[1], 2);
    ASSERT_EQ(vec[2], 3);
    ASSERT_EQ(vec[3], 4);
}

TEST(VectorTest, Iterate) {
    sm::SmallVector<int, 4> vec { 1, 2, 3, 4 };

    int i = 0;
    for (int val : vec) {
        ASSERT_EQ(val, i + 1);
        i++;
    }

    ASSERT_EQ(i, 4);
}

TEST(VectorTest, Index) {
    sm::SmallVector<int, 4> vec { 1, 2, 3, 4 };

    ASSERT_EQ(vec[0], 1);
    ASSERT_EQ(vec[1], 2);
    ASSERT_EQ(vec[2], 3);
    ASSERT_EQ(vec[3], 4);
}

TEST(VectorTest, CopyConst) {
    const sm::SmallVector<int, 4> vec { 1, 2, 3, 4 };

    sm::SmallVector<int, 4> copy = vec.clone();

    ASSERT_EQ(copy.size(), 4);
    ASSERT_FALSE(copy.isEmpty());
    ASSERT_EQ(copy[0], 1);
    ASSERT_EQ(copy[1], 2);
    ASSERT_EQ(copy[2], 3);
    ASSERT_EQ(copy[3], 4);
}

TEST(VectorTest, MoveConst) {
    sm::SmallVector<int, 4> vec { 1, 2, 3, 4 };

    const sm::SmallVector<int, 4> moved = std::move(vec);

    ASSERT_EQ(moved.size(), 4);
    ASSERT_FALSE(moved.isEmpty());
    ASSERT_EQ(moved[0], 1);
    ASSERT_EQ(moved[1], 2);
    ASSERT_EQ(moved[2], 3);
    ASSERT_EQ(moved[3], 4);
}

struct NonCopyable {
    NonCopyable() = default;
    NonCopyable(const NonCopyable &) = delete;
    NonCopyable(NonCopyable &&) = default;
    NonCopyable &operator=(const NonCopyable &) = delete;
    NonCopyable &operator=(NonCopyable &&) = default;
};

TEST(VectorTest, NonCopyable) {
    sm::SmallVector<NonCopyable, 4> vec;

    vec.push_back(NonCopyable());
    vec.push_back(NonCopyable());

    ASSERT_EQ(vec.size(), 2);
    ASSERT_FALSE(vec.isEmpty());
}

TEST(VectorTest, NonCopyableEmplace) {
    sm::SmallVector<NonCopyable, 4> vec;

    vec.emplace_back();
    vec.emplace_back();
    vec.emplace_back();
    vec.emplace_back();

    ASSERT_EQ(vec.size(), 4);
    ASSERT_FALSE(vec.isEmpty());
}
