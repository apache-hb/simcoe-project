#include "gtest_common.hpp"

#include "core/uuid.hpp"

using uuid = sm::uuid;

namespace chrono = std::chrono;

using namespace std::chrono_literals;

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
    chrono::system_clock::time_point birth = chrono::sys_days{2002y/7/27};
    uuid gen = uuid::v1(chrono::clock_cast<chrono::utc_clock>(birth), 0x1234, 0x7508BCA3230A8F12);
    EXPECT_EQ(uuid::eVersion1, gen.version());
    EXPECT_EQ(uuid::eRfc9562, gen.variant());

    char buffer[uuid::kStringSize + 1] = {};
    uuid::strfuid(buffer, gen);

    EXPECT_EQ(std::string(buffer), "74b9f099-b0a4-ff47-9234-8f12bca3230a");
}
