#define _CRT_SECURE_NO_WARNINGS

#include "test/db_test_common.hpp"

#include "common.hpp"

#include "db/connection.hpp"

#include "threads/threads.hpp"
#include "threads/topology.hpp"

using namespace sm;
using namespace sm::threads::detail;

TEST_CASE("CpuSet Geometry") {
    db::Environment sqlite = db::Environment::create(db::DbType::eSqlite3);
    db::Connection connection = sqlite.connect(makeSqliteTestDb("threads/info"));

    threads::saveThreadInfo(connection);

    SUCCEED("Saved thread info");
}

TEST_CASE("Hwloc Topology") {
    db::Environment sqlite = db::Environment::create(db::DbType::eSqlite3);
    db::Connection connection = sqlite.connect(makeSqliteTestDb("threads/hwloc"));

    if (auto topology = std::unique_ptr<HwlocTopology>(hwlocInit())) {
        topology->save(connection);
    } else {
        SKIP("Failed to initialize hwloc topology");
    }
}
