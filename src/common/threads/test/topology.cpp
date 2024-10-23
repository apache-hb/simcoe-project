#include "test/db_test_common.hpp"

#include "db/connection.hpp"

#include "threads/threads.hpp"
#include "backends/common.hpp"

using namespace sm;

using CpuInfoLibrary = threads::detail::CpuInfoLibrary;

TEST_CASE("CpuSet Geometry") {
    CpuInfoLibrary lib = CpuInfoLibrary::load();
    db::Environment sqlite = db::Environment::create(db::DbType::eSqlite3);
    db::Connection connection = sqlite.connect({ .host = "threadtest.db" });

    threads::saveThreadInfo(connection);

    SUCCEED("Saved thread info");
}
