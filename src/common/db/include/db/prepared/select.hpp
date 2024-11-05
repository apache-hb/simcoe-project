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

        std::vector<T> fetchAll() throws(DbException) {
            ResultSet result = db::throwIfFailed(mStatement.start());

            std::vector<T> values;

            do {
                values.emplace_back(result.getRow<T>());
            } while (!result.next().isDone());

            return values;
        }

        T fetchOne() throws(DbException) {
            ResultSet result = db::throwIfFailed(mStatement.start());

            if (result.isDone())
                throw DbException{DbError::noData()};

            return result.getRow<T>();
        }
    };
}
