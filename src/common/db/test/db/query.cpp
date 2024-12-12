#pragma once

#include "db/connection.hpp"
#include "db/environment.hpp"
#include "db/parameter.hpp"

// #include "tests.dao.hpp"

using namespace sm;
using namespace sm::db;

int main() {
    auto env = Environment::create(db::DbType::eSqlite3);
    auto db = env.connect({ .host = "test.db" });

    ResultSet result = db.execute("UPDATE test SET name = ", value("jeff"), " WHERE id = ", value(25));
}
