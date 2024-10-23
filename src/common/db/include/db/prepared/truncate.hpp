#pragma once

#include "db/results.hpp"
#include "db/statement.hpp"

#include "dao/dao.hpp"

namespace sm::db {
    class PreparedTruncate {
        friend Connection;

        PreparedStatement mStatement;

        PreparedTruncate(PreparedStatement statement) noexcept
            : mStatement(std::move(statement))
        { }

    public:
        SM_MOVE(PreparedTruncate, default);

        DbError tryTruncate() noexcept {
            return mStatement.execute();
        }

        void truncate() throws(DbException) {
            tryTruncate().throwIfFailed();
        }
    };
}
