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
        DbError finish() noexcept;

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

        void bindRowData(const dao::TableInfo& info, bool returning, const void *data) noexcept;

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
            mStatement.bindRowData(T::getTableInfo(), false, static_cast<const void*>(&value));
            return mStatement.execute();
        }
    };

    template<dao::HasPrimaryKey T>
    class PreparedInsertReturning {
        friend Connection;

        PreparedStatement mStatement;

        PreparedInsertReturning(PreparedStatement statement) noexcept
            : mStatement(std::move(statement))
        { }

    public:
        SM_MOVE(PreparedInsertReturning, default);

        DbResult<typename T::Id> tryInsert(const T& value) noexcept {
            const auto& info = T::getTableInfo();
            mStatement.bindRowData(info, true, static_cast<const void*>(&value));

            auto result = TRY_UNWRAP(mStatement.update());
            if (DbError error = result.next())
                return error;

            auto pk = result.get<typename T::Id>(info.primaryKey); // TODO: enforce primary key column

            if (DbError error = result.finish())
                return error;

            return pk;
        }
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

        DbResult<PreparedStatement> prepareInsertImpl(const dao::TableInfo& table) noexcept;
        DbResult<PreparedStatement> prepareInsertReturningPrimaryKeyImpl(const dao::TableInfo& table) noexcept;

        DbError insertOrUpdateImpl(const dao::TableInfo& table, const void *src) noexcept;
        DbError tryInsertOrUpdateAllImpl(const dao::TableInfo& table, const void *src, size_t count, size_t stride) noexcept;

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
        DbResult<PreparedInsert<T>> tryPrepareInsert() noexcept {
            auto stmt = TRY_RESULT(prepareInsertImpl(T::getTableInfo()));
            return PreparedInsert<T>{std::move(stmt)};
        }

        template<dao::DaoInterface T>
        PreparedInsert<T> prepareInsert() throws(DbException) {
            return throwIfFailed(tryPrepareInsert<T>());
        }

        template<dao::HasPrimaryKey T>
        DbResult<PreparedInsertReturning<T>> tryPrepareInsertReturningPrimaryKey() noexcept {
            auto stmt = TRY_RESULT(prepareInsertReturningPrimaryKeyImpl(T::getTableInfo()));
            return PreparedInsertReturning<T>{std::move(stmt)};
        }

        template<dao::HasPrimaryKey T>
        PreparedInsertReturning<T> prepareInsertReturningPrimaryKey() throws(DbException) {
            return throwIfFailed(tryPrepareInsertReturningPrimaryKey<T>());
        }

        template<dao::DaoInterface T>
        DbError tryInsert(const T& value) noexcept {
            auto stmt = TRY_UNWRAP(tryPrepareInsert<T>());
            return stmt.tryInsert(value);
        }

        template<dao::DaoInterface T>
        void insert(const T& value) throws(DbException) {
            prepareInsert<T>().tryInsert(value).throwIfFailed();
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
            auto stmt = TRY_UNWRAP(tryPrepareInsertReturningPrimaryKey<T>());
            return stmt.tryInsert(value);
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

        static DbResult<Environment> tryCreate(DbType type, const EnvConfig& config = EnvConfig{}) noexcept;

        static Environment create(DbType type, const EnvConfig& config = EnvConfig{}) {
            return throwIfFailed(tryCreate(type, config));
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
