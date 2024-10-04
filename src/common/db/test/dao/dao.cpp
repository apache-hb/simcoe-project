#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include "orm_test_common.hpp"

#include <thread>

#include "tests.dao.hpp"

using namespace std::chrono;
using namespace std::chrono_literals;

using Example = sm::dao::tests::Example;
using ExampleUpsert = sm::dao::tests::ExampleUpsert;
using TestInsertReturning = sm::dao::tests::TestInsertReturning;

struct TestCaseData {
    ConnectionConfig config;
    DbType type;
};

static constexpr TestCaseData kBackends[] = {
    TestCaseData {
        .config = kOracleConfig,
        .type = DbType::eOracleDB
    },
    TestCaseData {
        .config = kPostgresConfig,
        .type = DbType::ePostgreSQL
    },
    TestCaseData {
        .config = {
            .host = "testdao.db"
        },
        .type = DbType::eSqlite3
    }
};

static constexpr auto kExampleUpsertRowsFirst = std::to_array<sm::dao::tests::ExampleUpsert>({
    { 1, "1" },
    { 2, "two" },
    { 3, "three" },
    { 4, "four" },
    { 5, "five" },
    { 6, "six" },
    { 7, "seven" },
    { 8, "eight" },
    { 9, "nine" },
    { 10, "ten" },
});

static constexpr auto kExampleUpsertRowsSecond = std::to_array<sm::dao::tests::ExampleUpsert>({
    { 1, "un" },
    { 2, "deux" },
    { 3, "trois" },
    { 4, "quatre" },
    { 5, "cinq" },
    { 6, "six" },
    { 7, "sept" },
    { 8, "huit" },
    { 9, "neuf" },
    { 10, "dix" },
});

TEST_CASE("DAO") {
    auto backend = GENERATE(from_range(kBackends));

    GIVEN("A database backend") {
        if (!Environment::isSupported(backend.type)) {
            SKIP("Backend not supported");
        }

        auto env = Environment::create(backend.type);

        auto maybeDb = env.tryConnect(backend.config);
        if (!maybeDb.has_value()) {
            SKIP("Failed to connect to database");
        }

        auto conn = std::move(maybeDb.value());

        if (backend.type == DbType::eOracleDB) {
            conn.updateSql("ALTER SESSION SET EVENTS '10046 trace name context forever, level 12'");
        }

        THEN("Create table is idempotent") {
            // should be idempotent
            conn.createTable(Example::getTableInfo());
            conn.createTable(Example::getTableInfo());
            conn.createTable(Example::getTableInfo());

            conn.replaceTable(Example::getTableInfo());
        }

        THEN("Inserting a dao record succeeds") {
            db::DateTime now = sys_days(2024y/January/1d);

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
                .floating = 3.14,
                .doubleValue = 9999999999999999999.0,
                .startDate = now,

                .optName = "optional",
                .optYesno = true,
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
                CHECK(row.binary == Blob{0x01, 0x02, 0x03});
                CHECK(row.cxxint == 1);
                CHECK(row.cxxuint == 2);
                CHECK(row.cxxlong == 3);
                CHECK(row.cxxulong == 4);
                CHECK(std::abs(row.floating - 3.14) < 0.0001);
                CHECK(std::abs(row.doubleValue - 9999999999999999999.0) < 0.0001);
                CHECK(row.startDate == now);

                CHECK(row.optName == "optional");
                CHECK(row.optYesno == true);
                CHECK(!row.optCxxint.has_value());
                CHECK(!row.optCxxuint.has_value());

                count++;
            }

            CHECK(count == 1);
        }

        THEN("insert or update behaves correctly") {
            conn.replaceTable(ExampleUpsert::getTableInfo());
            {
                db::Transaction tx(&conn);
                for (const auto& row : kExampleUpsertRowsFirst) {
                    conn.insertOrUpdate(row);
                }
            }

            int count = 0;
            for (const auto& row : conn.selectAll<ExampleUpsert>()) {
                CHECK(kExampleUpsertRowsFirst[row.id - 1].name == row.name);
                count++;
            }

            CHECK(count == kExampleUpsertRowsFirst.size());

            {
                db::Transaction tx(&conn);
                for (const auto& row : kExampleUpsertRowsSecond) {
                    conn.insertOrUpdate(row);
                }
            }

            count = 0;
            for (const auto& row : conn.selectAll<ExampleUpsert>()) {
                CHECK(kExampleUpsertRowsSecond[row.id - 1].name == row.name);
                count++;
            }

            CHECK(count == kExampleUpsertRowsSecond.size());
        }

        THEN("insert returning behaves correctly") {
            conn.replaceTable(TestInsertReturning::getTableInfo());

            std::map<int, std::string> rows;
            const int totalCount = 10;

            for (int i = 0; i < totalCount; i++) {
                auto id = std::to_string(i);
                auto pk = conn.insertReturningPrimaryKey(TestInsertReturning { .name = id });
                rows[pk] = id;
            }

            int count = 0;
            for (const auto& row : conn.selectAll<TestInsertReturning>()) {
                CHECK(rows[row.id] == row.name);
                count++;
            }

            CHECK(count == totalCount);
        }
    }
}
