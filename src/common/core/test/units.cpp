#include <gtest/gtest.h>

#include "core/units.hpp"

TEST(UnitTest, RoundUp) {
    ASSERT_EQ(sm::roundup(0, 4), 4);
    ASSERT_EQ(sm::roundup(512, 256), 512);
    ASSERT_EQ(sm::roundup(0, 256), 256);
    ASSERT_EQ(sm::roundup(123, 256), 256);
}
