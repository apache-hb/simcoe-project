#pragma once

#include "db/results.hpp"
#include "db/statement.hpp"

#include "dao/dao.hpp"

namespace sm::db {
    template<dao::DaoInterface T>
    class PreparedSelect {
        friend Connection;

        PreparedStatement mStatement;

        PreparedSelect(PreparedStatement statement) noexcept
            : mStatement(std::move(statement))
        { }

    public:
        SM_MOVE(PreparedSelect, default);

        DbResult<std::vector<T>> tryFetchAll() noexcept {
            ResultSet result = TRY_RESULT(mStatement.start());

            std::vector<T> values;
            do {
                values.emplace_back(TRY_RESULT(result.getRow<T>()));
            } while (!result.next().isDone());

            return values;
        }

        DbResult<T> tryFetchOne() noexcept {
            ResultSet result = TRY_RESULT(mStatement.start());

            if (result.isDone())
                return std::unexpected{DbError::noData()};

            return result.getRow<T>();
        }

        std::vector<T> fetchAll() throws(DbException) {
            return throwIfFailed(tryFetchAll());
        }

        T fetchOne() throws(DbException) {
            return throwIfFailed(tryFetchOne());
        }
    };
}
