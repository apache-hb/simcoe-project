#include "test/gtest_common.hpp"

#include "core/adt/bitset.hpp"

namespace adt = sm::adt;

TEST(BitsetTest, Construction) {
    adt::BitSet bs{64};

    EXPECT_EQ(bs.popcount(), 0);
    EXPECT_EQ(bs.freecount(), 64);
    EXPECT_EQ(bs.getBitCapacity(), 64);
    EXPECT_FALSE(bs.any());
}

TEST(BitsetTest, MultiWordConstruction) {
    adt::BitSet bs{999};

    EXPECT_EQ(bs.popcount(), 0);
    EXPECT_EQ(bs.freecount(), 999);
    EXPECT_EQ(bs.getBitCapacity(), 999);
    EXPECT_FALSE(bs.any());
}

TEST(BitsetTest, SettingBits) {
    adt::BitSet bs{64};

    bs.set(0);
    bs.set(1);
    bs.set(2);
    bs.set(3);

    EXPECT_EQ(bs.popcount(), 4);
    EXPECT_EQ(bs.freecount(), 60);
    EXPECT_EQ(bs.getBitCapacity(), 64);
    EXPECT_TRUE(bs.any());
}

TEST(BitsetTest, ClearingBits) {
    adt::BitSet bs{64};

    bs.set(0);
    bs.set(1);
    bs.set(2);
    bs.set(3);

    bs.clear(1);
    bs.clear(3);

    EXPECT_EQ(bs.popcount(), 2);
    EXPECT_EQ(bs.freecount(), 62);
    EXPECT_EQ(bs.getBitCapacity(), 64);
    EXPECT_TRUE(bs.any());
}

TEST(BitsetTest, FlippingBits) {
    adt::BitSet bs{64};

    bs.flip(0);
    bs.flip(1);
    bs.flip(2);
    bs.flip(3);

    EXPECT_EQ(bs.popcount(), 4);
    EXPECT_EQ(bs.freecount(), 60);
    EXPECT_EQ(bs.getBitCapacity(), 64);
    EXPECT_TRUE(bs.any());
}

TEST(BitsetTest, ClearingAllBits) {
    adt::BitSet bs{64};

    bs.clear();

    EXPECT_EQ(bs.popcount(), 0);
    EXPECT_EQ(bs.freecount(), 64);
    EXPECT_EQ(bs.getBitCapacity(), 64);
    EXPECT_FALSE(bs.any());
}

TEST(BitsetTest, SettingHighBits) {
    adt::BitSet bs{999};

    bs.set(998);
    bs.set(997);
    bs.set(996);
    bs.set(995);

    EXPECT_EQ(bs.popcount(), 4);
    EXPECT_EQ(bs.freecount(), 995);
    EXPECT_EQ(bs.getBitCapacity(), 999);
    EXPECT_TRUE(bs.any());

    EXPECT_TRUE(bs.test(998));
    EXPECT_TRUE(bs.test(997));
    EXPECT_TRUE(bs.test(996));
    EXPECT_TRUE(bs.test(995));
}

TEST(BitsetTest, ClearingHighBits) {
    adt::BitSet bs{999};

    bs.set(998);
    bs.set(997);
    bs.set(996);
    bs.set(995);

    bs.clear(997);
    bs.clear(995);

    EXPECT_EQ(bs.popcount(), 2);
    EXPECT_EQ(bs.freecount(), 997);
    EXPECT_EQ(bs.getBitCapacity(), 999);
    EXPECT_TRUE(bs.any());
}

TEST(BitsetTest, FlippingHighBits) {
    adt::BitSet bs{999};

    bs.flip(998);
    bs.flip(997);
    bs.flip(996);
    bs.flip(995);

    EXPECT_EQ(bs.popcount(), 4);
    EXPECT_EQ(bs.freecount(), 995);
    EXPECT_EQ(bs.getBitCapacity(), 999);
    EXPECT_TRUE(bs.any());
}

TEST(BitsetTest, ClearingAllHighBits) {
    adt::BitSet bs{999};

    bs.clear();

    EXPECT_EQ(bs.popcount(), 0);
    EXPECT_EQ(bs.freecount(), 999);
    EXPECT_EQ(bs.getBitCapacity(), 999);
    EXPECT_FALSE(bs.any());
}

TEST(BitsetTest, ResizingBigger) {
    adt::BitSet bs{64};

    bs.resize(128);

    EXPECT_EQ(bs.popcount(), 0);
    EXPECT_EQ(bs.freecount(), 128);
    EXPECT_EQ(bs.getBitCapacity(), 128);
    EXPECT_FALSE(bs.any());
}

TEST(BitsetTest, ResizingSmaller) {
    adt::BitSet bs{999};

    bs.resize(500);

    EXPECT_EQ(bs.popcount(), 0);
    EXPECT_EQ(bs.freecount(), 500);
    EXPECT_EQ(bs.getBitCapacity(), 500);
    EXPECT_FALSE(bs.any());
}

TEST(BitsetTest, ResizingSame) {
    adt::BitSet bs{999};

    bs.resize(999);

    EXPECT_EQ(bs.popcount(), 0);
    EXPECT_EQ(bs.freecount(), 999);
    EXPECT_EQ(bs.getBitCapacity(), 999);
    EXPECT_FALSE(bs.any());
}

TEST(BitsetTest, ResizingToZero) {
    ASSERT_DEATH({
        adt::BitSet bs{999};

        bs.resize(0);
    }, "");
}
