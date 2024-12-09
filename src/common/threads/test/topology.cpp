#include "test/db_test_common.hpp"

#include "db/connection.hpp"

#include "threads/threads.hpp"
#include "threads/topology.hpp"

using namespace sm;
using namespace sm::threads;

TEST_CASE("CpuSet Geometry") {
    db::Environment sqlite = db::Environment::create(db::DbType::eSqlite3);
    db::Connection connection = sqlite.connect(makeSqliteTestDb("threads/info"));

    threads::saveThreadInfo(connection);

    SUCCEED("Saved thread info");
}

TEST_CASE("Hwloc Topology") {
    db::Environment sqlite = db::Environment::create(db::DbType::eSqlite3);
    db::Connection connection = sqlite.connect(makeSqliteTestDb("threads/hwloc"));

    if (auto topology = std::unique_ptr<HwlocTopology>(HwlocTopology::fromSystem())) {
        topology->save(connection);

        auto xml = topology->exportToXml();
        REQUIRE_FALSE(xml.empty());

        std::ofstream file("hwloc.xml");
        file << xml;
    } else {
        SKIP("Failed to initialize hwloc topology");
    }
}
