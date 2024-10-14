#include "test/common.hpp"
#include "test/db_test_common.hpp"

#include "db/connection.hpp"

#include "render/base/instance.hpp"

using namespace sm;

TEST_CASE("Save device info to db") {
    db::Environment sqlite = db::Environment::create(db::DbType::eSqlite3);
    db::Connection connection = sqlite.connect(makeSqliteTestDb("render/deviceinfo"));
    render::Instance instance{ render::InstanceConfig{} };

    render::saveAdapterInfo(instance, DXGI_FORMAT_R8G8B8A8_UNORM, connection);

    SUCCEED("Saved device info to db");
}
