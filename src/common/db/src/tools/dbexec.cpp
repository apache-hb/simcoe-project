#include "db/connection.hpp"
#include "test/db_test_common.hpp"

#include <map>

#include <fmtlib/format.h>
#include <fmt/ostream.h>

#include "core/fs.hpp"

using namespace sm;
using namespace sm::db;

static const std::map<std::string_view, DbType> kDbTypeMap = {
    { "oracle", DbType::eOracleDB },
    { "db2", DbType::eDB2 },
    { "sqlite", DbType::eSqlite3 },
    { "mysql", DbType::eMySQL },
    { "postgres", DbType::ePostgreSQL },
    { "mssql", DbType::eMSSQL }
};

static const std::map<std::string_view, AssumeRole> kRoleMap = {
    { "default", AssumeRole::eDefault },
    { "sysdba", AssumeRole::eSYSDBA },
    { "sysoper", AssumeRole::eSYSOPER }
};

enum Arg : int {
    eName = 0,
    eType = 1,
    eHost = 2,
    ePort = 3,
    eUser = 4,
    ePassword = 5,
    eDataSource = 6,
    eRole = 7,
    eScript = 8,

    eCount
};

int main(int argc, const char **argv) try {
    if (argc < eCount) {
        fmt::print(stderr, "Usage: {} <type> <host> <port> <user> <password> <database> <role> <script>\n", argv[0]);
        return 1;
    }

    std::string_view name = argv[eName];
    std::string_view dbtypename = argv[eType];
    if (!kDbTypeMap.contains(dbtypename)) {
        fmt::println(stderr, "Unknown database type: {}", dbtypename);
        return 1;
    }

    std::string_view rolename = argv[eRole];
    if (!kRoleMap.contains(rolename)) {
        fmt::println(stderr, "Unknown role: {}", rolename);
        return 1;
    }

    std::string_view script = argv[eScript];
    if (!fs::exists(script)) {
        fmt::println(stderr, "Script not found: {}", script);
        return 1;
    }

    std::vector<std::string> statements = loadSqlStatements(script);

    DbType type = kDbTypeMap.at(dbtypename);
    std::string host = argv[eHost];
    uint16_t port = std::stoi(argv[ePort]);
    std::string user = argv[eUser];
    std::string password = argv[ePassword];

    std::string database = argv[eDataSource];
    AssumeRole role = kRoleMap.at(rolename);

    auto env = Environment::create(type);
    ConnectionConfig config = {
        .port = port,
        .host = host,
        .user = user,
        .password = password,
        .database = database,
        .role = role,
    };

    auto db = env.connect(config);
    for (const std::string& sql : statements) {
        db.updateSql(sql);
    }

} catch (const DbException& e) {
    fmt::println(stderr, "Database error: {}", e.error());
    return 1;
} catch (const std::exception& e) {
    fmt::println(stderr, "Error: {}", e.what());
    return 1;
} catch (...) {
    fmt::println(stderr, "Unknown error");
    return 1;
}
