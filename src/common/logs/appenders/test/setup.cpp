#include "test/common.hpp"
#include "test/db_test_common.hpp"

#include "logger/logger.hpp"

#include "db/connection.hpp"

#include "logger/appenders/channels.hpp"
#include <random>

#include "fmt/std.h"

using namespace sm;
using namespace std::chrono_literals;

#define USE_ORACLE 0

#if USE_ORACLE

static constexpr db::ConnectionConfig kConfig = {
    .port = 1521,
    .host = "localhost",
    .user = "TEST_USER",
    .password = "TEST_USER",
    .database = "FREEPDB1",
    .timeout = 1s
};

static constexpr db::DbType kType = db::DbType::eOracleDB;

#else

static const db::ConnectionConfig kConfig = makeSqliteTestDb("logs/setup");

static constexpr db::DbType kType = db::DbType::eSqlite3;

#endif

static std::string createSalt(int length) {
    static constexpr char kChars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device random{};
    std::mt19937 gen(random());

    std::uniform_int_distribution<int> dist(0, sizeof(kChars) - 1);

    std::string salt;
    salt.reserve(length);

    for (int i = 0; i < length; i++) {
        salt.push_back(kChars[dist(gen)]);
    }

    return salt;
}

LOG_MESSAGE_CATEGORY(TestLog, "Tests");

TEST_CASE("Setup logging") {
    logs::create(logs::LoggingConfig { });

    auto env = db::Environment::create(kType, { .logQueries=true });
    try {
        logs::appenders::create(env.connect(kConfig));
    } catch (db::DbException& e) {
        for (const auto& frame : e.stacktrace()) {
            fmt::println(stderr, "[{}:{}] {}", frame.source_file(), frame.source_line(), frame.description());
        }
        FAIL(e.what());
    }

    GIVEN("a successfully setup logging environment") {
        THEN("a message can be logged") {
            LOG_INFO(TestLog, "Logging setup complete");

            LOG_INFO(TestLog, "Log message {0}", 5);

            LOG_INFO(TestLog, "Logging with {multiple} {parameters}", fmt::arg("multiple", 1), fmt::arg("parameters", false));

            LOG_INFO(TestLog, "Logging long string: {}", createSalt(128));

            for (int i = 0; i < 100; i++) {
                fs::path path = createSalt(64);
                LOG_INFO(TestLog, "Logging path: {}", path);
            }


            SUCCEED("No exceptions thrown");
        }
    }

    logs::appenders::destroy();
}
