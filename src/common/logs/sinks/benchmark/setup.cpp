#include <catch2/benchmark/catch_benchmark.hpp>

#include "test/db_test_common.hpp"

#include "logs/sinks/channels.hpp"
#include "logs/logger.hpp"

#include "core/win32.hpp"

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

LOG_MESSAGE_CATEGORY(TestLog, "Tests");

TEST_CASE("Setup logging") {
    SetProcessAffinityMask(GetCurrentProcess(), 0b1111'1111'1111'1111);
    logs::create(logs::LoggingConfig { });

    auto env = db::Environment::create(db::DbType::eSqlite3);

    logs::sinks::create(env.connect({.host="benchlogs.db"}));
    SUCCEED();

    BENCHMARK("Log plain message") {
        LOG_INFO(TestLog, "Benchmark logging message");
    };

    BENCHMARK("Log message with arguments") {
        LOG_INFO(TestLog, "Benchmark logging message with {arg}", fmt::arg("arg", 42));
    };

    logs::sinks::destroy();
}
