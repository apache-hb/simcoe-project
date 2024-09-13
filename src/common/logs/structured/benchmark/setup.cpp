#include <catch2/benchmark/catch_benchmark.hpp>
#include "test/common.hpp"

#include "db/connection.hpp"

#include "logs/structured/message.hpp"

using namespace sm;
using namespace std::chrono_literals;

// db::ConnectionConfig cfg {
//     .port = 1521,
//     .host = "localhost",
//     .user = "TEST_USER",
//     .password = "TEST_USER",
//     .database = "FREEPDB1",
//     .timeout = 1s
// };

TEST_CASE("Setup logging") {
    auto env = db::Environment::create(db::DbType::eSqlite3);
    auto conn = env.connect({.host="testlogs.db"});

    if (auto err = logs::structured::setup(conn)) {
        err.raise();
    }

    SUCCEED();

    BENCHMARK("Log plain message") {
        LOG_INFO("Benchmark logging message");
    };

    BENCHMARK("Log message with arguments") {
        LOG_INFO("Benchmark logging message with {arg}", 5);
    };

    logs::structured::cleanup();
}
