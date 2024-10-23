#pragma once

#include "db/results.hpp"
#include "db/statement.hpp"

#include "dao/dao.hpp"

namespace sm::db {
    class PreparedDrop {
        friend Connection;

        PreparedStatement mStatement;

        PreparedDrop(PreparedStatement statement) noexcept
            : mStatement(std::move(statement))
        { }

    public:
        SM_MOVE(PreparedDrop, default);

        DbError tryDrop() noexcept {
            return mStatement.execute();
        }

        void drop() throws(DbException) {
            tryDrop().throwIfFailed();
        }
    };
}
