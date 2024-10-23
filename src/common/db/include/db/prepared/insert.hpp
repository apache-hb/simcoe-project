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

        DbError tryInsert(const T& value) noexcept {
            if (DbError error = bindRowToStatement(mStatement, T::table(), false, static_cast<const void*>(&value)))
                return error;

            return mStatement.execute();
        }

        void insert(const T& value) throws(DbException) {
            tryInsert(value).throwIfFailed();
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

        DbResult<PrimaryKey> tryInsert(const T& value) {
            const auto& info = T::table();
            if (DbError error = bindRowToStatement(mStatement, info, true, static_cast<const void*>(&value)))
                return std::unexpected{error};

            ResultSet result = TRY_UNWRAP(mStatement.start());

            auto pk = result.getReturn<PrimaryKey>(info.primaryKeyIndex()); // TODO: enforce primary key column

            if (DbError error = result.execute())
                return error;

            return pk;
        }

        PrimaryKey insert(const T& value) throws(DbException) {
            return throwIfFailed(tryInsert(value));
        }
    };
}
