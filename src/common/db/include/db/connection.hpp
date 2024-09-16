#pragma once

#include <simcoe_db_config.h>

#include "db/bind.hpp"
#include "db/core.hpp"
#include "db/error.hpp"
#include "db/results.hpp"

#include "core/core.hpp"
#include "core/macros.hpp"

#include "dao/dao.hpp"

namespace sm::db {
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
        DbResult<ResultSet> start() noexcept;

        DbError execute() noexcept;
        DbError step() noexcept;

        DbError close() noexcept;

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

        using PrimaryKey = typename T::Id;

        PreparedStatement mStatement;

        PreparedInsertReturning(PreparedStatement statement) noexcept
            : mStatement(std::move(statement))
        { }

    public:
        SM_MOVE(PreparedInsertReturning, default);

        DbResult<PrimaryKey> tryInsert(const T& value) noexcept {
            const auto& info = T::getTableInfo();
            mStatement.bindRowData(info, true, static_cast<const void*>(&value));

            auto result = TRY_UNWRAP(mStatement.start());

            auto pk = result.get<PrimaryKey>(info.primaryKey); // TODO: enforce primary key column

            if (DbError error = result.execute())
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
        DbResult<PreparedStatement> prepareInsertOrUpdateImpl(const dao::TableInfo& table) noexcept;
        DbResult<PreparedStatement> prepareInsertReturningPrimaryKeyImpl(const dao::TableInfo& table) noexcept;

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

        template<dao::DaoInterface T>
        DbResult<PreparedInsert<T>> tryPrepareInsertOrUpdate() noexcept {
            auto stmt = TRY_RESULT(prepareInsertOrUpdateImpl(T::getTableInfo()));
            return PreparedInsert<T>{std::move(stmt)};
        }

        template<dao::DaoInterface T>
        PreparedInsert<T> prepareInsertOrUpdate() throws(DbException) {
            return throwIfFailed(tryPrepareInsertOrUpdate<T>());
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
            auto stmt = TRY_UNWRAP(tryPrepareInsertOrUpdate<T>());
            return stmt.tryInsert(value);
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
