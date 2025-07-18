#include "gtest_common.hpp"

#include "core/uuid.hpp"

#include <array>

using uuid = sm::uuid;
using rfc9562_clock = sm::detail::rfc9562_clock;

namespace chrono = std::chrono;

using namespace std::chrono_literals;

static constexpr chrono::system_clock::time_point kBirth = chrono::sys_days{2002y/7/27};

TEST(UuidTest, Rfc9562Clock) {
    rfc9562_clock::time_point there = chrono::floor<rfc9562_clock::duration>(chrono::clock_cast<rfc9562_clock>(kBirth));
    chrono::system_clock::time_point back = chrono::clock_cast<chrono::system_clock>(there);

    EXPECT_EQ(kBirth, back);

    // 419 years between 2002y/7/27 and 1582y/10/15
    EXPECT_EQ(chrono::floor<chrono::years>(there.time_since_epoch()), chrono::years{419});

    EXPECT_EQ(chrono::floor<chrono::days>(there.time_since_epoch()), chrono::days{153322});
}

TEST(UuidTest, Rfc9562ClockEpoch) {
    rfc9562_clock::time_point there = chrono::floor<rfc9562_clock::duration>(chrono::clock_cast<rfc9562_clock>(kBirth));

    EXPECT_EQ(there.time_since_epoch().count(), 132470208000000000ull);
}

class UuidVersionTest : public testing::TestWithParam<uint8_t> {};

INSTANTIATE_TEST_SUITE_P(
    Version, UuidVersionTest,
    testing::Range<uint8_t>(1, 9),
    [](const testing::TestParamInfo<uint8_t>& info) {
        return fmt::format("Version{}", info.param);
    });

TEST_P(UuidVersionTest, Version) {
    uuid it = uuid::nil();
    EXPECT_EQ(0, it.version());

    uint8_t param = GetParam();

    it.setVersion(param);
    EXPECT_EQ(param, it.version());
}

TEST(UuidTest, Variant) {
    uuid it = uuid::nil();
    EXPECT_EQ(0, it.variant());

    it.setVariant(uuid::eDCE);

    char buffer[uuid::kStringSize + 1] = {};
    uuid::strfuid(buffer, it);

    EXPECT_EQ(uuid::eDCE, it.variant()) << buffer;
}

static void TestRoundTrip(uuid before) {
    char buffer[uuid::kStringSize + 1] = {};
    uuid::strfuid(buffer, before);

    uuid after;
    EXPECT_TRUE(uuid::parse(buffer, after));

    EXPECT_EQ(before, after) << buffer;
}

TEST(UuidTest, V1) {
    chrono::utc_clock::time_point today = chrono::clock_cast<chrono::utc_clock>(chrono::sys_days{2024y/5/6} + 15h + 14min + 47s + 73ms);
    sm::MacAddress node = 0xBC'A3'23'0A'8F'12;
    uuid gen = uuid::v1(today, 0x1234, node);
    char buffer[uuid::kStringSize + 1] = {};
    uuid::strfuid(buffer, gen);

    EXPECT_EQ(uuid::eVersion1, gen.version()) << buffer;
    EXPECT_EQ(uuid::eDCE, gen.variant()) << buffer;

    EXPECT_EQ(std::string_view(buffer), "10b11760-bb0b-11ef-9234-bca3230a8f12");

    EXPECT_EQ(0x1234, gen.v1ClockSeq()) << buffer;
    EXPECT_EQ(node, gen.v1Node()) << buffer;
    EXPECT_EQ(today, gen.v1Time()) << buffer;

    TestRoundTrip(gen);
}

TEST(UuidTest, V6) {
    static constexpr chrono::system_clock::time_point initial = chrono::sys_days{2024y/12/17} + 6h + 16min + 40s;

    chrono::utc_clock::time_point birth = chrono::clock_cast<chrono::utc_clock>(initial);
    sm::MacAddress node = 0xBC'A3'23'0A'8F'12;
    uuid gen = uuid::v6(birth, 3386, node);

    char buffer[uuid::kStringSize + 1] = {};
    uuid::strfuid(buffer, gen);

    EXPECT_EQ(uuid::eVersion6, gen.version()) << buffer;
    EXPECT_EQ(uuid::eDCE, gen.variant()) << buffer;

    EXPECT_EQ(std::string_view(buffer), "1efbc3e7-a711-6400-8d3a-bca3230a8f12");

    EXPECT_EQ(3386, gen.v6ClockSeq()) << buffer;
    EXPECT_EQ(node, gen.v6Node()) << buffer;
    EXPECT_EQ(birth, gen.v6Time()) << buffer;

    TestRoundTrip(gen);
}

TEST(UuidTest, V7) {
    static constexpr chrono::system_clock::time_point initial = chrono::sys_days{2024y/12/17} + 6h + 4min + 48s + 984ms;

    uint8_t random[10] = { 0x1F, 0xF3, 0x5A, 0x19, 0x92, 0x80, 0x42, 0x40, 0x43, 0xC };
    uuid gen = uuid::v7(initial, random);

    char buffer[uuid::kStringSize + 1] = {};
    uuid::strfuid(buffer, gen);

    EXPECT_EQ(uuid::eVersion7, gen.version()) << buffer;
    EXPECT_EQ(uuid::eDCE, gen.variant()) << buffer;

    // TODO: this is 11ms off :(
    EXPECT_EQ(std::string_view(buffer), "0193d338-17d8-7ff3-9a19-92804240430c");

    EXPECT_EQ(initial, gen.v7Time()) << buffer;

    TestRoundTrip(gen);
}

class UuidParseTest : public testing::TestWithParam<std::tuple<std::string, bool, uuid>> {};

INSTANTIATE_TEST_SUITE_P(
    ParseBasic, UuidParseTest,
    testing::Values(
        std::make_tuple("10b11760-bb0b-11ef-9234-bca3230a8f12", true, uuid::of(0x10b11760, 0xbb0b, 0x11ef, 0x9234, 0xbca3230a8f12)),
        std::make_tuple("1efbc3e7-a711-6400-8d3a-bca3230a8f12", true, uuid::of(0x1efbc3e7, 0xa711, 0x6400, 0x8d3a, 0xbca3230a8f12)),
        std::make_tuple("0193d338-17d8-7ff3-9a19-92804240430c", true, uuid::of(0x0193d338, 0x17d8, 0x7ff3, 0x9a19, 0x92804240430c)),
        std::make_tuple("0193d33817d87ff39a1992804240430c", false, uuid::nil()),
        std::make_tuple("", false, uuid::nil())
    ),
    [](const testing::TestParamInfo<std::tuple<std::string, bool, uuid>>& info) {
        std::string title = std::get<0>(info.param);
        std::replace(title.begin(), title.end(), '-', '_');
        return title.empty() ? "Empty" : title;
    });

TEST_P(UuidParseTest, ParseBasic) {
    auto [input, expected, result] = GetParam();

    // its the callers responsibility to ensure the buffer is large enough
    std::array<char, uuid::kStringSize> buffer;
    std::copy(input.begin(), input.end(), buffer.begin());

    uuid out = uuid::nil();
    EXPECT_EQ(expected, uuid::parse(buffer.data(), out));

    if (expected) {
        EXPECT_EQ(result, out);
    }
}

class UuidParseMsTest : public testing::TestWithParam<std::tuple<std::string, bool, uuid>> {};

INSTANTIATE_TEST_SUITE_P(
    ParseMicrosoft, UuidParseMsTest,
    testing::Values(
        std::make_tuple("{10b11760-bb0b-11ef-9234-bca3230a8f12}", true, uuid::of(0x10b11760, 0xbb0b, 0x11ef, 0x9234, 0xbca3230a8f12)),
        std::make_tuple("{1efbc3e7-a711-6400-8d3a-bca3230a8f12}", true, uuid::of(0x1efbc3e7, 0xa711, 0x6400, 0x8d3a, 0xbca3230a8f12)),
        std::make_tuple("{0193d338-17d8-7ff3-9a19-92804240430c}", true, uuid::of(0x0193d338, 0x17d8, 0x7ff3, 0x9a19, 0x92804240430c)),
        // microsoft uuids must have both braces to parse successfully
        std::make_tuple("{c24bc4e9-d566-484a-97a2-0dc59675563a", false, uuid::nil()),
        std::make_tuple("e5876a2c-7ab6-419c-be88-b1dce9027059}", false, uuid::nil()),
        std::make_tuple("cee5413e-4387-4f76-9ea2-5ae62d213677", false, uuid::nil()),
        std::make_tuple("{0193d33817d87ff39a1992804240430c}", false, uuid::nil()),
        std::make_tuple("", false, uuid::nil())
    ),
    [](const testing::TestParamInfo<std::tuple<std::string, bool, uuid>>& info) {
        std::string title = std::get<0>(info.param);
        std::replace(title.begin(), title.end(), '-', '_');
        title.erase(std::remove(title.begin(), title.end(), '{'), title.end());
        title.erase(std::remove(title.begin(), title.end(), '}'), title.end());
        return title.empty() ? "Empty" : fmt::format("MS_{}", title);
    });

TEST_P(UuidParseMsTest, ParseMicrosoft) {
    auto [input, expected, result] = GetParam();

    // its the callers responsibility to ensure the buffer is large enough
    std::array<char, uuid::kMicrosoftStringSize> buffer;
    std::copy(input.begin(), input.end(), buffer.begin());

    uuid out = uuid::nil();
    EXPECT_EQ(expected, uuid::parseMicrosoft(buffer.data(), out));

    if (expected) {
        EXPECT_EQ(result, out);
    }
}

TEST(UuidTest, ParseNoPartialWrite) {
    {
        uuid out = uuid::nil();
        EXPECT_FALSE(uuid::parse("im not a uuid", out));
        EXPECT_EQ(uuid::nil(), out);
    }

    {
        uuid out = uuid::max();
        EXPECT_FALSE(uuid::parse("im not a uuid either", out));
        EXPECT_EQ(uuid::max(), out);
    }
}

TEST(UuidTest, ParseMicrosoftNoPartialWrite) {
    {
        uuid out = uuid::nil();
        EXPECT_FALSE(uuid::parseMicrosoft("im not a uuid", out));
        EXPECT_EQ(uuid::nil(), out);
    }

    {
        uuid out = uuid::max();
        EXPECT_FALSE(uuid::parseMicrosoft("im not a uuid either", out));
        EXPECT_EQ(uuid::max(), out);
    }
}
