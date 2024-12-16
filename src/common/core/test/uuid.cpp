#include "gtest_common.hpp"

#include "core/uuid.hpp"

using uuid = sm::uuid;
using rfc9562_clock = sm::detail::rfc9562_clock;

namespace chrono = std::chrono;

using namespace std::chrono_literals;

static constexpr chrono::system_clock::time_point kBirth = chrono::sys_days{2002y/7/27};

TEST(UuidTest, Rfc9562Clock) {
    rfc9562_clock::time_point there = chrono::clock_cast<rfc9562_clock>(kBirth);
    chrono::system_clock::time_point back = chrono::clock_cast<chrono::system_clock>(there);

    EXPECT_EQ(kBirth, back);

    // 419 years between 2002y/7/27 and 1582y/10/15
    EXPECT_EQ(chrono::floor<chrono::years>(there.time_since_epoch()), chrono::years{419});

    EXPECT_EQ(chrono::floor<chrono::days>(there.time_since_epoch()), chrono::days{153322});
}

TEST(UuidTest, Rfc9562ClockEpoch) {
    rfc9562_clock::time_point there = chrono::clock_cast<rfc9562_clock>(kBirth);

    EXPECT_EQ(there.time_since_epoch().count(), 132470208000000000ull);
}

TEST(UuidTest, Version) {
    uuid it = uuid::nil();
    EXPECT_EQ(0, it.version());

    for (uint8_t i = 1; i <= 8; i++) {
        it.setVersion(i);
        EXPECT_EQ(i, it.version());
    }
}

TEST(UuidTest, Variant) {
    uuid it = uuid::nil();
    EXPECT_EQ(0, it.variant());

    it.setVariant(uuid::eRfc9562);

    char buffer[uuid::kStringSize + 1] = {};
    uuid::strfuid(buffer, it);

    EXPECT_EQ(uuid::eRfc9562, it.variant()) << buffer;
}

TEST(UuidTest, V1) {
    chrono::utc_clock::time_point birth = chrono::clock_cast<chrono::utc_clock>(chrono::sys_days{2024y/5/6} + 15h + 14min + 47s + 73ms);
    sm::MacAddress node = 0xBC'A3'23'0A'8F'12;
    uuid gen = uuid::v1(birth, 0x1234, node);
    char buffer[uuid::kStringSize + 1] = {};
    uuid::strfuid(buffer, gen);

    EXPECT_EQ(uuid::eVersion1, gen.version()) << buffer;
    EXPECT_EQ(uuid::eRfc9562, gen.variant()) << buffer;

    EXPECT_EQ(std::string(buffer), "10b11760-bb0b-11ef-9234-bca3230a8f12");

    // 08c81760-bb0b-11ef-9234-bca3230a8f12 - good time

    // 10b11760-bb0b-11ef-9234-bca3230a8f12 - bad time

    EXPECT_EQ(0x1234, gen.v1ClockSeq()) << buffer;
    EXPECT_EQ(node, gen.v1Node()) << buffer;
    EXPECT_EQ(birth, gen.v1Time()) << buffer;
}

// intervals: 0b00000111101111000010111011101101100000000101111011000100010000
// time:      0b00000111101111000010111011101101100000000101111011000100010000