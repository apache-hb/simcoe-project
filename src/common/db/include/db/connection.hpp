#pragma once

#include <simcoe_db_config.h>

#include "db/bind.hpp"
#include "db/core.hpp"
#include "db/error.hpp"

#include "core/core.hpp"
#include "core/macros.hpp"

#include "dao/dao.hpp"

namespace sm::db {
    /// @brief Represents a result set of a query.
    ///
    /// @note Not internally synchronized.
    /// @note Resetting the statement that produced the result set will invalidate the result set.
    /// @note Columns are 0-indexed.
    class ResultSet {
        friend PreparedStatement;

        detail::StmtHandle mImpl;

        ResultSet(detail::StmtHandle impl) noexcept
            : mImpl(std::move(impl))
        { }

    public:
        DbError next() noexcept;

        int getColumnCount() const noexcept;
        DbResult<ColumnInfo> getColumnInfo(int index) const noexcept;

        DbResult<double> getDouble(int index) noexcept;
        DbResult<int64> getInt(int index) noexcept;
        DbResult<bool> getBool(int index) noexcept;
        DbResult<std::string_view> getString(int index) noexcept;
        DbResult<Blob> getBlob(int index) noexcept;

        DbResult<double> getDouble(std::string_view column) noexcept;
        DbResult<int64> getInt(std::string_view column) noexcept;
        DbResult<bool> getBool(std::string_view column) noexcept;
        DbResult<std::string_view> getString(std::string_view column) noexcept;
        DbResult<Blob> getBlob(std::string_view column) noexcept;

        template<typename T>
        DbResult<T> get(std::string_view column) noexcept;

        template<typename T>
        DbResult<T> get(int index) noexcept;

        template<std::integral T>
        DbResult<T> get(int column) noexcept {
            return getInt(column)
                .transform([](auto it) {
                    return static_cast<T>(it);
                });
        }

        template<std::floating_point T>
        DbResult<T> get(int column) noexcept {
            return getDouble(column)
                .transform([](auto it) {
                    return static_cast<T>(it);
                });
        }

        template<std::integral T>
        DbResult<T> get(std::string_view column) noexcept {
            return getInt(column)
                .transform([](auto it) {
                    return static_cast<T>(it);
                });
        }

        template<std::floating_point T>
        DbResult<T> get(std::string_view column) noexcept {
            return getDouble(column)
                .transform([](auto it) {
                    return static_cast<T>(it);
                });
        }
    };

#define RESULT_SET_GET_IMPL(type, method) \
    template<> \
    inline DbResult<type> ResultSet::get<type>(std::string_view column) noexcept { \
        return method(column); \
    } \
    template<> \
    inline DbResult<type> ResultSet::get<type>(int index) noexcept { \
        return method(index); \
    }

    RESULT_SET_GET_IMPL(bool, getBool);
    RESULT_SET_GET_IMPL(std::string_view, getString);
    RESULT_SET_GET_IMPL(Blob, getBlob);

#undef RESULT_SET_GET_IMPL

    class PreparedStatement {
        friend Connection;

        detail::StmtHandle mImpl;
        Connection *mConnection;
        StatementType mType;

        PreparedStatement(detail::IStatement *impl, Connection *connection, StatementType type) noexcept
            : mImpl({impl, &detail::destroyStatement})
            , mConnection(connection)
            , mType(type)
        { }

    public:
        SM_MOVE(PreparedStatement, default);

        BindPoint bind(std::string_view name) noexcept;
        void bind(std::string_view name, int64 value) noexcept { bind(name).toInt(value); }
        void bind(std::string_view name, std::string_view value) noexcept { bind(name).toString(value); }
        void bind(std::string_view name, double value) noexcept { bind(name).toDouble(value); }
        void bind(std::string_view name, std::nullptr_t) noexcept { bind(name).toNull(); }

        DbResult<ResultSet> select() noexcept;
        DbResult<ResultSet> update() noexcept;

        DbError execute() noexcept;
        DbError step() noexcept;

        DbError close() noexcept;
        DbError reset() noexcept;

        [[nodiscard]]
        StatementType type() const noexcept { return mType; }
    };

    class Connection {
        friend Environment;

        detail::ConnectionHandle mImpl;

        bool mAutoCommit;

        DbResult<PreparedStatement> sqlPrepare(std::string_view sql, StatementType type) noexcept;

        Connection(detail::IConnection *impl, const ConnectionConfig& config) noexcept
            : mImpl(impl)
            , mAutoCommit(config.autoCommit)
        { }

        DbError insertImpl(const dao::TableInfo& table, const void *src) noexcept;
        DbError insertOrUpdateImpl(const dao::TableInfo& table, const void *src) noexcept;
        DbError tryInsertOrUpdateAllImpl(const dao::TableInfo& table, const void *src, size_t count, size_t stride) noexcept;

        DbError insertReturningPrimaryKeyImpl(const dao::TableInfo& table, const void *src, void *dst) noexcept;

    public:
        SM_MOVE(Connection, default);

        DbError begin() noexcept;
        DbError rollback() noexcept;
        DbError commit() noexcept;

        void setAutoCommit(bool enabled) noexcept { mAutoCommit = enabled; }

        [[nodiscard]]
        bool autoCommit() const noexcept { return mAutoCommit; }

        DbError tryCreateTable(const dao::TableInfo& table) noexcept;

        void createTable(const dao::TableInfo& table) throws(DbException) {
            tryCreateTable(table).throwIfFailed();
        }

        template<dao::DaoInterface T>
        DbError tryInsert(const T& value) noexcept {
            return insertImpl(T::getTableInfo(), static_cast<const void*>(&value));
        }

        template<dao::DaoInterface T>
        void insert(const T& value) throws(DbException) {
            tryInsert(value).throwIfFailed();
        }

        template<dao::DaoInterface T>
        DbError tryInsertOrUpdate(const T& value) noexcept {
            return insertOrUpdateImpl(T::getTableInfo(), static_cast<const void*>(&value));
        }

        template<dao::DaoInterface T>
        void insertOrUpdate(const T& value) throws(DbException) {
            tryInsertOrUpdate(value).throwIfFailed();
        }

        template<dao::HasPrimaryKey T>
        DbResult<typename T::Id> tryInsertReturningPrimaryKey(const T& value) noexcept {
            typename T::Id primaryKey{};
            const void *src = static_cast<const void*>(&value);
            void *dst = static_cast<void*>(&primaryKey);
            if (DbError error = insertReturningPrimaryKeyImpl(T::getTableInfo(), src, dst))
                return error;

            return primaryKey;
        }

        template<dao::HasPrimaryKey T>
        typename T::Id insertReturningPrimaryKey(const T& value) throws(DbException) {
            return throwIfFailed(tryInsertReturningPrimaryKey(value));
        }

        DbResult<ResultSet> select(std::string_view sql) noexcept;
        DbResult<ResultSet> update(std::string_view sql) noexcept;

        DbResult<PreparedStatement> dqlPrepare(std::string_view sql) noexcept;
        DbResult<PreparedStatement> dmlPrepare(std::string_view sql) noexcept;
        DbResult<PreparedStatement> ddlPrepare(std::string_view sql) noexcept;
        DbResult<PreparedStatement> dclPrepare(std::string_view sql) noexcept;

        DbResult<bool> tableExists(std::string_view name) noexcept;

        DbResult<Version> clientVersion() const noexcept;
        DbResult<Version> serverVersion() const noexcept;
    };

    class Environment {
        friend Connection;

        detail::EnvHandle mImpl;

        Environment(detail::IEnvironment *impl) noexcept
            : mImpl(impl)
        { }

        detail::IEnvironment *impl() noexcept { return mImpl.get(); }

    public:
        SM_MOVE(Environment, default);

        [[nodiscard]]
        static bool isSupported(DbType type) noexcept;

        static DbResult<Environment> tryCreate(DbType type) noexcept;
        static Environment create(DbType type) {
            return throwIfFailed(tryCreate(type));
        }

        DbResult<Connection> tryConnect(const ConnectionConfig& config) noexcept;
        Connection connect(const ConnectionConfig& config) {
            auto result = tryConnect(config);
            if (!result.has_value())
                throw DbConnectionException{result.error(), config};

            return std::move(*result);
        }
    };
}
