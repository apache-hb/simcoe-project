#pragma once

#include "db/results.hpp"
#include "db/statement.hpp"

#include "dao/dao.hpp"

namespace sm::db {
    template<dao::DaoInterface T>
    class PreparedInsert {
        friend Connection;

        PreparedStatement mStatement;

        PreparedInsert(PreparedStatement statement) noexcept
            : mStatement(std::move(statement))
        { }

    public:
        SM_MOVE(PreparedInsert, default);

        DbError tryInsert(const T& value) noexcept try {
            insert(value);
            return DbError::ok();
        } catch (const DbException& e) {
            return e.error();
        }

        void insert(const T& value) throws(DbException) {
            bindRowToStatement(mStatement, T::table(), false, static_cast<const void*>(&value));
            mStatement.execute().throwIfFailed();
        }
    };

    template<dao::HasPrimaryKey T>
    class PreparedInsertReturning {
        friend Connection;

        using PrimaryKey = typename T::PrimaryKey;

        PreparedStatement mStatement;

        PreparedInsertReturning(PreparedStatement statement) noexcept
            : mStatement(std::move(statement))
        { }

    public:
        SM_MOVE(PreparedInsertReturning, default);

        DbResult<PrimaryKey> tryInsert(const T& value) try {
            return insert(value);
        } catch (const DbException& e) {
            return e.error();
        }

        PrimaryKey insert(const T& value) throws(DbException) {
            const auto& info = T::table();
            bindRowToStatement(mStatement, info, true, static_cast<const void*>(&value));

            ResultSet result = db::throwIfFailed(mStatement.start());

            auto pk = result.getReturn<PrimaryKey>(info.primaryKeyIndex()); // TODO: enforce primary key column

            result.execute().throwIfFailed();

            return pk;
        }
    };
}
