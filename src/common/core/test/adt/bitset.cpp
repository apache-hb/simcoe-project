#include "test/common.hpp"

#include "core/adt/bitset.hpp"

TEST_CASE("bitset construction") {
    GIVEN("a bitset") {
        sm::BitSet bs{64};

        THEN("it initializes correctly") {
            CHECK(bs.popcount() == 0);
            CHECK(bs.freecount() == 64);
            CHECK(bs.getBitCapacity() == 64);
            CHECK_FALSE(bs.any());
        }
    }

    GIVEN("a multi word bitset") {
        sm::BitSet bs{999};

        THEN("it initializes correctly") {
            CHECK(bs.popcount() == 0);
            CHECK(bs.freecount() == 999);
            CHECK(bs.getBitCapacity() == 999);
            CHECK_FALSE(bs.any());
        }
    }
}

TEST_CASE("bitset operations") {
    GIVEN("a bitset") {
        sm::BitSet bs{64};

        THEN("setting bits works") {
            bs.set(0);
            bs.set(1);
            bs.set(2);
            bs.set(3);

            CHECK(bs.popcount() == 4);
            CHECK(bs.freecount() == 60);
            CHECK(bs.getBitCapacity() == 64);
            CHECK(bs.any());
        }

        THEN("clearing bits works") {
            bs.set(0);
            bs.set(1);
            bs.set(2);
            bs.set(3);

            bs.clear(1);
            bs.clear(3);

            CHECK(bs.popcount() == 2);
            CHECK(bs.freecount() == 62);
            CHECK(bs.getBitCapacity() == 64);
            CHECK(bs.any());
        }

        THEN("flipping bits works") {
            bs.flip(0);
            bs.flip(1);
            bs.flip(2);
            bs.flip(3);

            CHECK(bs.popcount() == 4);
            CHECK(bs.freecount() == 60);
            CHECK(bs.getBitCapacity() == 64);
            CHECK(bs.any());
        }

        THEN("clearing all bits works") {
            bs.clear();

            CHECK(bs.popcount() == 0);
            CHECK(bs.freecount() == 64);
            CHECK(bs.getBitCapacity() == 64);
            CHECK_FALSE(bs.any());
        }
    }

    GIVEN("a bitset") {
        sm::BitSet bs{999};

        THEN("setting high bits") {
            bs.set(998);
            bs.set(997);
            bs.set(996);
            bs.set(995);

            CHECK(bs.popcount() == 4);
            CHECK(bs.freecount() == 995);
            CHECK(bs.getBitCapacity() == 999);
            CHECK(bs.any());

            CHECK(bs.test(998));
            CHECK(bs.test(997));
            CHECK(bs.test(996));
            CHECK(bs.test(995));
        }

        THEN("clearing high bits") {
            bs.set(998);
            bs.set(997);
            bs.set(996);
            bs.set(995);

            bs.clear(997);
            bs.clear(995);

            CHECK(bs.popcount() == 2);
            CHECK(bs.freecount() == 997);
            CHECK(bs.getBitCapacity() == 999);
            CHECK(bs.any());
        }

        THEN("flipping high bits") {
            bs.flip(998);
            bs.flip(997);
            bs.flip(996);
            bs.flip(995);

            CHECK(bs.popcount() == 4);
            CHECK(bs.freecount() == 995);
            CHECK(bs.getBitCapacity() == 999);
            CHECK(bs.any());
        }

        THEN("clearing all high bits") {
            bs.clear();

            CHECK(bs.popcount() == 0);
            CHECK(bs.freecount() == 999);
            CHECK(bs.getBitCapacity() == 999);
            CHECK_FALSE(bs.any());
        }
    }
}

TEST_CASE("resizing a bitset") {
    GIVEN("a bitset") {
        sm::BitSet bs{64};

        THEN("resizing to a smaller size") {
            bs.resize(32);

            CHECK(bs.popcount() == 0);
            CHECK(bs.freecount() == 32);
            CHECK(bs.getBitCapacity() == 32);
            CHECK_FALSE(bs.any());
        }

        THEN("resizing to a larger size") {
            bs.resize(128);

            CHECK(bs.popcount() == 0);
            CHECK(bs.freecount() == 128);
            CHECK(bs.getBitCapacity() == 128);
            CHECK_FALSE(bs.any());
        }
    }

    GIVEN("a bitset") {
        sm::BitSet bs{999};

        THEN("resizing to a smaller size") {
            bs.resize(500);

            CHECK(bs.popcount() == 0);
            CHECK(bs.freecount() == 500);
            CHECK(bs.getBitCapacity() == 500);
            CHECK_FALSE(bs.any());
        }

        THEN("resizing to a larger size") {
            bs.resize(2000);

            CHECK(bs.popcount() == 0);
            CHECK(bs.freecount() == 2000);
            CHECK(bs.getBitCapacity() == 2000);
            CHECK_FALSE(bs.any());
        }
    }
}
