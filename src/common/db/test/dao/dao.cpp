#include <catch2/generators/catch_generators.hpp>
#include "orm_test_common.hpp"

#include "tests.dao.hpp"

using namespace std::chrono_literals;

using Example = sm::dao::tests::Example;

struct TestCaseData {
    ConnectionConfig config;
    DbType type;
};

static constexpr TestCaseData kBackends[] = {
    TestCaseData {
        .config = {
            .port = 1521,
            .host = "localhost",
            .user = "TEST_USER",
            .password = "TEST_USER",
            .database = "FREEPDB1",
            .timeout = 1s
        },
        .type = DbType::eOracleDB
    },
    TestCaseData {
        .config = {
            .port = 5432,
            .host = "localhost",
            .user = "TEST_USER",
            .password = "TEST_USER",
            .database = "TESTDB",
            .timeout = 1s
        },
        .type = DbType::ePostgreSQL
    },
    TestCaseData {
        .config = {
            .host = "testdao.db"
        },
        .type = DbType::eSqlite3
    }
};

TEST_CASE("DAO") {
    for (const auto& backend : kBackends) {
        if (!Environment::isSupported(backend.type)) {
            continue;
        }

        auto env = Environment::create(backend.type);

        auto maybeDb = env.tryConnect(backend.config);
        if (!maybeDb.has_value()) {
            continue;
        }

        auto conn = std::move(maybeDb.value());

        // should be idempotent
        conn.createTable(Example::getTableInfo());
        conn.createTable(Example::getTableInfo());
        conn.createTable(Example::getTableInfo());

        if (conn.tableExists(Example::getTableInfo()).value_or(false)) {
            conn.drop<Example>();
        }

        conn.createTable(Example::getTableInfo());

        Example example {
            .id = 1,
            .name = "example",
            .yesno = true,
            .x = 1,
            .y = 50,
            .binary = { 0x01, 0x02, 0x03 },
            .cxxint = 1,
            .cxxuint = 2,
            .cxxlong = 3,
            .cxxulong = 4,
            .floating = 3.14
        };

        conn.truncate<Example>();

        conn.insert(example);

        int count = 0;
        for (const auto& row : conn.selectAll<Example>()) {
            CHECK(row.id == 1);
            CHECK(row.name == "example");
            CHECK(row.yesno == true);
            CHECK(row.x == 1);
            CHECK(row.y == 50);
            CHECK(row.binary == std::vector<uint8_t>{0x01, 0x02, 0x03});
            CHECK(row.cxxint == 1);
            CHECK(row.cxxuint == 2);
            CHECK(row.cxxlong == 3);
            CHECK(row.cxxulong == 4);
            CHECK(std::abs(row.floating - 3.14) < 0.0001);

            count++;
        }

        CHECK(count == 1);
    }

}
