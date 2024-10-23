#pragma once

#include "db/results.hpp"
#include "db/statement.hpp"

#include "dao/dao.hpp"

namespace sm::db {
template<dao::DaoInterface T>
    class PreparedUpdate {
        friend Connection;

        PreparedStatement mStatement;

        PreparedUpdate(PreparedStatement statement) noexcept
            : mStatement(std::move(statement))
        { }

    public:
        SM_MOVE(PreparedUpdate, default);

        DbError tryUpdate(const T& value) noexcept {
            if (DbError error = bindRowToStatement(mStatement, T::table(), false, static_cast<const void*>(&value)))
                return error;
            return mStatement.execute();
        }

        void update(const T& value) throws(DbException) {
            tryUpdate(value).throwIfFailed();
        }
    };
}
