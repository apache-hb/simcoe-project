#pragma once

#include "db/db.hpp"
#include "db/error.hpp"
#include "db/results.hpp"
#include "db/statement.hpp"

#include "core/macros.hpp"

#include "db/prepared/insert.hpp"
#include "db/prepared/select.hpp"
#include "db/prepared/update.hpp"
#include "db/prepared/truncate.hpp"
#include "db/prepared/drop.hpp"

namespace sm::db {
    class Connection {
        friend Environment;
        friend ResultSet;

        detail::ConnectionHandle mImpl;

        bool mAutoCommit;

        Connection(detail::IConnection *impl, const ConnectionConfig& config) noexcept
            : mImpl(impl)
            , mAutoCommit(config.autoCommit)
        { }

        PreparedStatement prepareInsertImpl(const dao::TableInfo& table);
        PreparedStatement prepareInsertOrUpdateImpl(const dao::TableInfo& table);
        PreparedStatement prepareInsertReturningPrimaryKeyImpl(const dao::TableInfo& table);

        PreparedStatement prepareTruncateImpl(const dao::TableInfo& table);

        PreparedStatement prepareUpdateAllImpl(const dao::TableInfo& table);

        PreparedStatement prepareSelectAllImpl(const dao::TableInfo& table);

        PreparedStatement prepareDropTableImpl(const dao::TableInfo& table);

        detail::IConnection *impl() noexcept { return mImpl.get(); }

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

        /// @brief create a table
        /// @param table table to create
        /// @return true if the table was created, false if it already exists
        bool createTable(const dao::TableInfo& table) throws(DbException);

        DbError tryDropTable(const dao::TableInfo& table) noexcept;

        ///
        /// prepare insert
        ///

        template<dao::DaoInterface T>
        DbResult<PreparedInsert<T>> tryPrepareInsert() noexcept try {
            return prepareInsert<T>();
        } catch (const DbException& e) {
            return std::unexpected{e.error()};
        }

        template<dao::DaoInterface T>
        PreparedInsert<T> prepareInsert() throws(DbException) {
            auto stmt = prepareInsertImpl(T::table());
            return PreparedInsert<T>{std::move(stmt)};
        }

        template<dao::DaoInterface T>
        DbResult<PreparedInsert<T>> tryPrepareInsertOrUpdate() noexcept try {
            return prepareInsertOrUpdate<T>();
        } catch (const DbException& e) {
            return std::unexpected{e.error()};
        }

        template<dao::DaoInterface T>
        PreparedInsert<T> prepareInsertOrUpdate() throws(DbException) {
            auto stmt = prepareInsertOrUpdateImpl(T::table());
            return PreparedInsert<T>{std::move(stmt)};
        }

        template<dao::HasPrimaryKey T>
        DbResult<PreparedInsertReturning<T>> tryPrepareInsertReturningPrimaryKey() noexcept try {
            return prepareInsertReturningPrimaryKey<T>();
        } catch (const DbException& e) {
            return std::unexpected{e.error()};
        }

        template<dao::HasPrimaryKey T>
        PreparedInsertReturning<T> prepareInsertReturningPrimaryKey() throws(DbException) {
            auto stmt = prepareInsertReturningPrimaryKeyImpl(T::table());
            return PreparedInsertReturning<T>{std::move(stmt)};
        }

        ///
        /// insert
        ///

        template<dao::DaoInterface T>
        DbError tryInsert(const T& value) noexcept try {
            insert<T>(value);
            return DbError::ok();
        } catch (const DbException& e) {
            return e.error();
        }

        template<dao::DaoInterface T>
        void insert(const T& value) throws(DbException) {
            auto stmt = prepareInsert<T>();
            stmt.insert(value);
        }

        template<dao::DaoInterface T>
        DbError tryInsertOrUpdate(const T& value) noexcept try {
            insertOrUpdate<T>(value);
            return DbError::ok();
        } catch (const DbException& e) {
            return e.error();
        }

        template<dao::DaoInterface T>
        void insertOrUpdate(const T& value) throws(DbException) {
            auto stmt = prepareInsertOrUpdate<T>();
            stmt.insert(value);
        }

        template<dao::HasPrimaryKey T>
        DbResult<typename T::PrimaryKey> tryInsertReturningPrimaryKey(const T& value) noexcept try {
            return insertReturningPrimaryKey<T>(value);
        } catch (const DbException& e) {
            return std::unexpected{e.error()};
        }

        template<dao::HasPrimaryKey T>
        typename T::PrimaryKey insertReturningPrimaryKey(const T& value) throws(DbException) {
            PreparedInsertReturning<T> stmt = prepareInsertReturningPrimaryKey<T>();
            return stmt.insert(value);
        }

        ///
        /// prepare truncate
        ///

        template<dao::DaoInterface T>
        DbResult<PreparedTruncate> tryPrepareTruncate() noexcept try {
            return prepareTruncate<T>();
        } catch (const DbException& e) {
            return std::unexpected{e.error()};
        }

        template<dao::DaoInterface T>
        PreparedTruncate prepareTruncate() throws(DbException) {
            auto stmt = prepareTruncateImpl(T::table());
            return PreparedTruncate{std::move(stmt)};
        }

        ///
        /// truncate
        ///

        template<dao::DaoInterface T>
        DbError tryTruncate() noexcept {
            auto stmt = TRY_UNWRAP(tryPrepareTruncate<T>());
            return stmt.tryTruncate();
        }

        template<dao::DaoInterface T>
        void truncate() throws(DbException) {
            truncate(T::table());
        }

        void truncate(const dao::TableInfo& info) throws(DbException) {
            auto stmt = prepareTruncateImpl(info);
            stmt.execute().throwIfFailed();
        }

        ///
        /// prepare drop
        ///

        template<dao::DaoInterface T>
        DbResult<PreparedDrop> tryPrepareDropTable() noexcept try {
            return prepareDropTable<T>();
        } catch (const DbException& e) {
            return std::unexpected{e.error()};
        }

        template<dao::DaoInterface T>
        PreparedDrop prepareDropTable() throws(DbException) {
            auto stmt = prepareDropImpl(T::table());
            return PreparedDrop{std::move(stmt)};
        }

        ///
        /// drop
        ///

        template<dao::DaoInterface T>
        DbError tryDropTable() noexcept {
            auto stmt = TRY_UNWRAP(tryPrepareDropTable<T>());
            return stmt.execute();
        }

        template<dao::DaoInterface T>
        void dropTable() throws(DbException) {
            dropTable(T::table());
        }

        void dropTable(const dao::TableInfo& info) throws(DbException);
        void dropTableIfExists(const dao::TableInfo& info) throws(DbException);

        ///
        /// prepare update
        ///

        template<dao::DaoInterface T>
        DbResult<PreparedUpdate<T>> tryPrepareUpdateAll() noexcept {
            auto stmt = TRY_RESULT(prepareUpdateAllImpl(T::table()));
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
        DbResult<PreparedSelect<T>> tryPrepareSelectAll() noexcept try {
            return prepareSelectAll<T>();
        } catch (const DbException& e) {
            return std::unexpected{e.error()};
        }

        template<dao::DaoInterface T>
        PreparedSelect<T> prepareSelectAll() throws(DbException) {
            auto stmt = prepareSelectAllImpl(T::table());
            return PreparedSelect<T>{std::move(stmt)};
        }

        ///
        /// select
        ///

        template<dao::DaoInterface T>
        DbResult<std::vector<T>> trySelectAll() try {
            auto stmt = TRY_RESULT(tryPrepareSelectAll<T>());
            return stmt.tryFetchAll();
        } catch (const DbException& e) {
            return std::unexpected{e.error()};
        }

        template<dao::DaoInterface T>
        std::vector<T> selectAll() throws(DbException) {
            PreparedSelect<T> stmt = prepareSelectAll<T>();
            return stmt.fetchAll();
        }

        template<dao::DaoInterface T>
        DbResult<T> trySelectOne() try {
            return selectOne<T>();
        } catch (const DbException& e) {
            return std::unexpected{e.error()};
        }

        template<dao::DaoInterface T>
        T selectOne() throws(DbException) {
            PreparedSelect<T> stmt = prepareSelectAll<T>();
            return stmt.fetchOne();
        }

        template<dao::DaoInterface T>
        std::vector<T> selectAllWhere(std::string_view sql) throws(DbException) {
            auto stmt = prepareQuery(sql);
            PreparedSelect<T> prepared{std::move(stmt)};
            return prepared.fetchAll();
        }

        template<dao::DaoInterface T>
        T selectOneWhere(std::string_view sql) throws(DbException) {
            auto stmt = prepareQuery(sql);
            PreparedSelect<T> prepared{std::move(stmt)};
            return prepared.fetchOne();
        }

        ///
        /// utils
        ///

        void replaceTable(const dao::TableInfo& info) throws(DbException);

        ///
        /// raw access
        ///

        DbResult<ResultSet> trySelectSql(std::string_view sql) noexcept;
        DbError tryUpdateSql(std::string_view sql) noexcept;

        ResultSet selectSql(std::string_view sql) throws(DbException) {
            return throwIfFailed(trySelectSql(sql));
        }

        void updateSql(std::string_view sql) throws(DbException) {
            tryUpdateSql(sql).throwIfFailed();
        }

        DbResult<PreparedStatement> tryPrepareStatement(std::string_view sql, StatementType type) noexcept;

        PreparedStatement prepareStatement(std::string_view sql, StatementType type) throws(DbException);

        DbResult<PreparedStatement> tryPrepareQuery(std::string_view sql) noexcept;
        DbResult<PreparedStatement> tryPrepareUpdate(std::string_view sql) noexcept;
        DbResult<PreparedStatement> tryPrepareDefine(std::string_view sql) noexcept;
        DbResult<PreparedStatement> tryPrepareControl(std::string_view sql) noexcept;

        PreparedStatement prepareQuery(std::string_view sql) throws(DbException) {
            return throwIfFailed(tryPrepareQuery(sql));
        }

        PreparedStatement prepareUpdate(std::string_view sql) throws(DbException) {
            return throwIfFailed(tryPrepareUpdate(sql));
        }

        PreparedStatement prepareDefine(std::string_view sql) throws(DbException) {
            return throwIfFailed(tryPrepareDefine(sql));
        }

        PreparedStatement prepareControl(std::string_view sql) throws(DbException) {
            return throwIfFailed(tryPrepareControl(sql));
        }

        bool tableExists(std::string_view name) throws(DbException);

        bool tableExists(const dao::TableInfo& info) throws(DbException);

        /// @brief Check if a user exists
        /// @param name user name
        /// @param fallback value to return if the database does not support users
        /// @return true if the user exists
        /// @note If the database does not support users, this will always return the value of fallback
        bool userExists(std::string_view name, bool fallback = false) throws(DbException);

        /// @brief Does this database support the concept of a user?
        /// @return true if the database supports users
        /// @note sqlite does not support users.
        bool hasUsers() const noexcept;

        Version clientVersion() const;
        Version serverVersion() const;
    };
}
