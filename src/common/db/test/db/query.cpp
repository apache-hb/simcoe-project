#pragma once

#include "db/connection.hpp"
#include "db/environment.hpp"
#include "db/parameter.hpp"

#include "dao/query.hpp"

// #include "tests.dao.hpp"

using namespace sm;
using namespace sm::db;
using namespace sm::dao;

int main() {
    auto env = Environment::create(db::DbType::eSqlite3);
    auto db = env.connect({ .host = "test.db" });

    {
        dao::Query query = Select<uint64_t, std::string>("id", "name")
            .from(table("test"))
            .where(Query::ofColumn("id") == 25);

        ResultSet result = db.execute(query);
    }

    {
        PreparedStatement stmt = db.prepare("SELECT * FROM test WHERE id = ", param<uint64_t>("id"));
        stmt.bind("id") = 25ull;
        for (const auto& row : db::throwIfFailed(stmt.start())) {
            fmt::println("id: {}, name: {}", row.get<uint64_t>("id").value(), row.get<std::string_view>("name").value());
        }
    }
}
