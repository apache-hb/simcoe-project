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

    template<dao::HasPrimaryKey T>
    class PreparedSelectByPrimaryKey {
        friend Connection;

        PreparedStatement mStatement;

        PreparedSelectByPrimaryKey(PreparedStatement statement) noexcept
            : mStatement(std::move(statement))
        { }

    public:
        SM_MOVE(PreparedSelectByPrimaryKey, default);

        T fetchOne(typename T::PrimaryKey pk) throws(DbException) {
            mStatement.bind("id").to(pk);
            ResultSet result = db::throwIfFailed(mStatement.start());

            if (result.isDone())
                throw DbException{DbError::noData()};

            return result.getRow<T>();
        }
    };
}
