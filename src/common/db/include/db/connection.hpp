#pragma once

#include <simcoe_db_config.h>

#include "db/core.hpp"
#include "db/error.hpp"
#include "db/results.hpp"
#include "db/statement.hpp"

#include "core/macros.hpp"

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
            bindRowToStatement(mStatement, T::getTableInfo(), false, static_cast<const void*>(&value));
            return mStatement.execute();
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

        DbResult<PrimaryKey> tryInsert(const T& value) noexcept {
            const auto& info = T::getTableInfo();
            bindRowToStatement(mStatement, info, true, static_cast<const void*>(&value));

            auto result = TRY_UNWRAP(mStatement.start());

            auto pk = result.get<PrimaryKey>(info.primaryKeyIndex()); // TODO: enforce primary key column

            if (DbError error = result.execute())
                return error;

            return pk;
        }
    };

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
    };

    template<dao::DaoInterface T>
    class PreparedUpdate {
        friend Connection;

        PreparedStatement mStatement;

        PreparedUpdate(PreparedStatement statement) noexcept
            : mStatement(std::move(statement))
        { }

    public:
        SM_MOVE(PreparedUpdate, default);

        DbError tryUpdate(const T& value) noexcept {
            bindRowToStatement(mStatement, T::getTableInfo(), false, static_cast<const void*>(&value));
            return mStatement.execute();
        }
    };

    class Connection {
        friend Environment;

        detail::ConnectionHandle mImpl;

        bool mAutoCommit;

        Connection(detail::IConnection *impl, const ConnectionConfig& config) noexcept
            : mImpl(impl)
            , mAutoCommit(config.autoCommit)
        { }

        DbResult<PreparedStatement> prepareInsertImpl(const dao::TableInfo& table) noexcept;
        DbResult<PreparedStatement> prepareInsertOrUpdateImpl(const dao::TableInfo& table) noexcept;
        DbResult<PreparedStatement> prepareInsertReturningPrimaryKeyImpl(const dao::TableInfo& table) noexcept;

        DbResult<PreparedStatement> prepareUpdateAllImpl(const dao::TableInfo& table) noexcept;

        DbResult<PreparedStatement> prepareSelectAllImpl(const dao::TableInfo& table) noexcept;

        DbError tryInsertOrUpdateAllImpl(const dao::TableInfo& table, const void *src, size_t count, size_t stride) noexcept;

    public:
        SM_MOVE(Connection, default);

        DbError begin() noexcept;
        DbError rollback() noexcept;
        DbError commit() noexcept;

        DbError tryBegin() noexcept { return begin(); }
        DbError tryRollback() noexcept { return rollback(); }
        DbError tryCommit() noexcept { return commit(); }

        void setAutoCommit(bool enabled) noexcept { mAutoCommit = enabled; }

        [[nodiscard]]
        bool autoCommit() const noexcept { return mAutoCommit; }

        DbError tryCreateTable(const dao::TableInfo& table) noexcept;

        void createTable(const dao::TableInfo& table) throws(DbException) {
            tryCreateTable(table).throwIfFailed();
        }

        ///
        /// prepare insert
        ///

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

        ///
        /// insert
        ///

        template<dao::DaoInterface T>
        DbError tryInsert(const T& value) noexcept {
            auto stmt = TRY_UNWRAP(tryPrepareInsert<T>());
            return stmt.tryInsert(value);
        }

        template<dao::DaoInterface T>
        void insert(const T& value) throws(DbException) {
            tryInsert(value).throwIfFailed();
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
        DbResult<typename T::PrimaryKey> tryInsertReturningPrimaryKey(const T& value) noexcept {
            auto stmt = TRY_UNWRAP(tryPrepareInsertReturningPrimaryKey<T>());
            return stmt.tryInsert(value);
        }

        template<dao::HasPrimaryKey T>
        typename T::PrimaryKey insertReturningPrimaryKey(const T& value) throws(DbException) {
            return throwIfFailed(tryInsertReturningPrimaryKey(value));
        }

        ///
        /// prepare update
        ///

        template<dao::DaoInterface T>
        DbResult<PreparedUpdate<T>> tryPrepareUpdateAll() noexcept {
            auto stmt = TRY_RESULT(prepareUpdateAllImpl(T::getTableInfo()));
            return PreparedUpdate<T>{std::move(stmt)};
        }

        template<dao::DaoInterface T>
        PreparedUpdate<T> prepareUpdateAll() throws(DbException) {
            return throwIfFailed(tryPrepareUpdateAll<T>());
        }

        ///
        /// updates
        ///

        template<dao::DaoInterface T>
        DbError tryUpdateAll(const T& value) noexcept {
            auto stmt = TRY_UNWRAP(tryPrepareUpdateAll<T>());
            return stmt.tryUpdate(value);
        }

        template<dao::DaoInterface T>
        void updateAll(const T& value) throws(DbException) {
            tryUpdateAll(value).throwIfFailed();
        }

        ///
        /// prepare select
        ///

        template<dao::DaoInterface T>
        DbResult<PreparedSelect<T>> tryPrepareSelectAll() noexcept {
            auto stmt = TRY_RESULT(prepareSelectAllImpl(T::getTableInfo()));
            return PreparedSelect<T>{std::move(stmt)};
        }

        template<dao::DaoInterface T>
        PreparedSelect<T> prepareSelectAll() throws(DbException) {
            return throwIfFailed(tryPrepareSelectAll<T>());
        }

        ///
        /// select
        ///

        template<dao::DaoInterface T>
        DbResult<std::vector<T>> trySelectAll() noexcept {
            auto stmt = TRY_RESULT(tryPrepareSelectAll<T>());
            return stmt.tryFetchAll();
        }

        template<dao::DaoInterface T>
        std::vector<T> selectAll() throws(DbException) {
            return throwIfFailed(trySelectAll<T>());
        }

        template<dao::DaoInterface T>
        DbResult<T> trySelectOne() noexcept {
            auto stmt = TRY_RESULT(tryPrepareSelectAll<T>());
            return stmt.tryFetchOne();
        }

        template<dao::DaoInterface T>
        T selectOne() throws(DbException) {
            return throwIfFailed(trySelectOne<T>());
        }

        DbResult<ResultSet> trySelectSql(std::string_view sql) noexcept;
        DbResult<ResultSet> tryUpdateSql(std::string_view sql) noexcept;

        ResultSet selectSql(std::string_view sql) throws(DbException) {
            return throwIfFailed(trySelectSql(sql));
        }

        ResultSet updateSql(std::string_view sql) throws(DbException) {
            return throwIfFailed(tryUpdateSql(sql));
        }

        DbResult<PreparedStatement> tryPrepareStatement(std::string_view sql, StatementType type) noexcept;

        PreparedStatement prepareStatement(std::string_view sql, StatementType type) throws(DbException) {
            return throwIfFailed(tryPrepareStatement(sql, type));
        }

        DbResult<PreparedStatement> prepareQuery(std::string_view sql) noexcept;
        DbResult<PreparedStatement> prepareUpdate(std::string_view sql) noexcept;
        DbResult<PreparedStatement> prepareDefine(std::string_view sql) noexcept;
        DbResult<PreparedStatement> prepareControl(std::string_view sql) noexcept;

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
