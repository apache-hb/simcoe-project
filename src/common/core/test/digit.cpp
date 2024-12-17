#include "gtest_common.hpp"

#include "core/digit.hpp"
#include "core/endian.hpp"

#include <fmtlib/format.h>

using namespace sm;

TEST(DigitTest, LoadStore) {
    uint48_t value = 0x123456789ABC;
    uint48_t copy = value;

    EXPECT_EQ(value, copy);
    EXPECT_EQ(0x123456789ABC, copy);
}

TEST(DigitTest, Storage) {
    uint48_t value = uint48_t(0x1234);

    // the memory layout of the octets should be the same as the current platform
    // as if there was support for integers of the same size.

    uint8_t expected[6];
    uint64_t expectedValue = 0x1234;
    memcpy(expected, &expectedValue, sizeof(expected));

    EXPECT_EQ(value.octets[0], expected[0]);
    EXPECT_EQ(value.octets[1], expected[1]);
    EXPECT_EQ(value.octets[2], expected[2]);
    EXPECT_EQ(value.octets[3], expected[3]);
    EXPECT_EQ(value.octets[4], expected[4]);
    EXPECT_EQ(value.octets[5], expected[5]);
}

TEST(DigitTest, BigEndian) {
    be<uint48_t> value = uint48_t(0x123456789ABC);

    // the memory layout of the octets should be big endian

    uint8_t expected[6] = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC };

    uint48_t underlying = value.underlying;

    EXPECT_EQ(underlying.octets[0], expected[0]);
    EXPECT_EQ(underlying.octets[1], expected[1]);
    EXPECT_EQ(underlying.octets[2], expected[2]);
    EXPECT_EQ(underlying.octets[3], expected[3]);
    EXPECT_EQ(underlying.octets[4], expected[4]);
    EXPECT_EQ(underlying.octets[5], expected[5]);
}

TEST(DigitTest, LittleEndian) {
    le<uint48_t> value = uint48_t(0x123456789ABC);

    // the memory layout of the octets should be little endian

    uint8_t expected[6] = { 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12 };

    EXPECT_EQ(value.underlying.octets[0], expected[0]);
    EXPECT_EQ(value.underlying.octets[1], expected[1]);
    EXPECT_EQ(value.underlying.octets[2], expected[2]);
    EXPECT_EQ(value.underlying.octets[3], expected[3]);
    EXPECT_EQ(value.underlying.octets[4], expected[4]);
    EXPECT_EQ(value.underlying.octets[5], expected[5]);
}

TEST(DigitTest, ByteSwap) {
    uint48_t value = uint48_t(0x123456789ABC).byteswap();

    uint8_t expected[6];
    uint64_t expectedValue = 0x123456789ABC;
    memcpy(expected, &expectedValue, sizeof(expected));

    EXPECT_EQ(value.octets[0], expected[5]);
    EXPECT_EQ(value.octets[1], expected[4]);
    EXPECT_EQ(value.octets[2], expected[3]);
    EXPECT_EQ(value.octets[3], expected[2]);
    EXPECT_EQ(value.octets[4], expected[1]);
    EXPECT_EQ(value.octets[5], expected[0]);
}
