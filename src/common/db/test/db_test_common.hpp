#pragma once

#include "test/common.hpp"

// IWYU pragma: begin_exports
#include "db/connection.hpp"
#include "db/transaction.hpp"
// IWYU pragma: end_exports

using namespace sm;
using namespace sm::db;
using namespace std::chrono_literals;

static constexpr ConnectionConfig kOracleConfig = {
    .port = 1521,
    .host = "localhost",
    .user = "TEST_USER",
    .password = "TEST_USER",
    .database = "FREEPDB1",
    .timeout = 1s,
};

static constexpr ConnectionConfig kDB2Config = {
    .port = 50000,
    .host = "localhost",
    .user = "db2inst1",
    .password = "db2inst1",
    .database = "testdb",
    .timeout = 1s,
};

static constexpr ConnectionConfig kPostgresConfig = {
    .port = 5432,
    .host = "localhost",
    .user = "TEST_USER",
    .password = "TEST_USER",
    .database = "TESTDB",
    .timeout = 1s,
};

void checkError(const DbError& err);

template<typename T>
auto getValue(DbResult<T> result, std::string_view msg = "") {
    if (result.has_value()) {
        SUCCEED(msg);
        return std::move(result.value());
    }

    checkError(result.error());
    throw std::runtime_error("unexpected error");
}

ConnectionConfig makeSqliteTestDb(const std::string& name, const ConnectionConfig& extra = {});
