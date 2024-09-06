#pragma once

#include "db/connection.hpp"

#include "dao/dao.hpp"

namespace sm::dao {

    db::DbError selectImpl(db::Connection& connection, const TableInfo& info, void *data) noexcept;
    db::DbError insertImpl(db::Connection& connection, const TableInfo& info, const void *data) noexcept;
    db::DbError insertGetPrimaryKeyImpl(db::Connection& connection, const TableInfo& info, const void *data, void *generated) noexcept;
    db::DbError createTableImpl(db::Connection& connection, const TableInfo& info) noexcept;

    template<DaoInterface T>
    db::DbResult<T> select(db::Connection& connection) noexcept {
        alignas(T) char buffer[sizeof(T)];
        if (db::DbError error = selectImpl(connection, T::getTableInfo(), buffer))
            return error;

        return *static_cast<T*>(buffer);
    }

    template<DaoInterface T>
    db::DbError insert(db::Connection& connection, const T& data) noexcept {
        return insertImpl(connection, T::getTableInfo(), static_cast<const void*>(&data));
    }

    template<typename T> requires (DaoInterface<T> && HasPrimaryKey<T>)
    db::DbResult<typename T::Id> insertGetPrimaryKey(db::Connection& connection, const T& data) noexcept {
        typename T::Id generated{};
        const void *src = static_cast<const void*>(&data);
        void *dst = static_cast<void*>(&generated);
        if (db::DbError error = insertGetPrimaryKeyImpl(connection, T::getTableInfo(), src, dst))
            return error;

        return generated;
    }

    template<DaoInterface T>
    db::DbError createTable(db::Connection& connection) noexcept {
        return createTableImpl(connection, T::getTableInfo());
    }

    template<DaoInterface T>
    bool tableExists(db::Connection& connection) noexcept {
        const TableInfo& info = T::getTableInfo();
        return connection.tableExists(info.name).value_or(false);
    }
}
