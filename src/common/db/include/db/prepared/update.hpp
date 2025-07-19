#pragma once

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

        DbError tryUpdate(const T& value) try {
            update(value);
            return DbError::ok();
        } catch (const DbException& e) {
            return e.error();
        }

        void update(const T& value) throws(DbException) {
            bindRowToStatement(mStatement, T::table(), false, static_cast<const void*>(&value));
            mStatement.execute().throwIfFailed();
        }
    };
}
