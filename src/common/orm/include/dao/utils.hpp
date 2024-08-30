#pragma once

#include "orm/connection.hpp"

#include "dao/dao.hpp"

namespace sm::dao {
    db::DbError selectImpl(db::Connection& connection, const TableInfo& info, void *data) noexcept;
    db::DbError insertImpl(db::Connection& connection, const TableInfo& info, const void *data) noexcept;
    db::DbError createTableImpl(db::Connection& connection, const TableInfo& info) noexcept;

    template<DaoInterface T>
    std::expected<T, db::DbError> select(db::Connection& connection) noexcept {
        alignas(T) char buffer[sizeof(T)];
        if (db::DbError error = selectImpl(connection, T::getTableInfo(), buffer))
            return error;

        return *reinterpret_cast<T*>(buffer);
    }

    template<DaoInterface T>
    db::DbError insert(db::Connection& connection, const T& data) noexcept {
        return insertImpl(connection, T::getTableInfo(), reinterpret_cast<const void*>(&data));
    }

    template<DaoInterface T>
    db::DbError createTable(db::Connection& connection) noexcept {
        return createTableImpl(connection, T::getTableInfo());
    }

    template<DaoInterface T>
    bool tableExists(db::Connection& connection) noexcept {
        return connection.tableExists(T::kTableName);
    }
}
