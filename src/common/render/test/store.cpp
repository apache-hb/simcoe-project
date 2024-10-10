#include "test/common.hpp"

#include "db/connection.hpp"

#include "render/base/instance.hpp"

using namespace sm;

TEST_CASE("Save device info to db") {
    db::Environment sqlite = db::Environment::create(db::DbType::eSqlite3);
    db::Connection connection = sqlite.connect({ .host = "rendertest.db" });
    render::Instance instance{ render::InstanceConfig{} };

    render::saveAdapterInfo(instance, DXGI_FORMAT_R8G8B8A8_UNORM, connection);

    SUCCEED("Saved device info to db");
}
