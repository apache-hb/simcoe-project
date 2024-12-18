#pragma once

#include "db/db.hpp"
#include "db/error.hpp"

#include "dao/info.hpp"

namespace sm::db::detail {
    class IEnvironment {
    public:
        virtual ~IEnvironment() = default;

        /** Lifecycle */

        virtual bool close() noexcept = 0;

        /** Connection */

        virtual IConnection *connect(const ConnectionConfig& config) throws(DbException) = 0;
    };

    struct ConnectionInfo {
        Version clientVersion;
        Version serverVersion;

        /// If this database doesnt have a boolean column type,
        /// this is the equivalent type to use.
        DataType boolType = DataType::eBoolean;

        /// If this database doesnt have a datetime column type,
        /// this is the equivalent type to use.
        DataType dateTimeType = DataType::eDateTime;

        /// Does this database require additional COMMENT ON statements
        /// after creating a table?
        /// sqlite and mysql support this via either adding comments
        /// to the create table statement, or by using inline COMMENT ON
        /// statements.
        /// all other databases - to my knowledge - require additional
        /// COMMENT ON statements.
        bool hasCommentOn = false;

        /// True if this database driver supports :named placeholders.
        /// oracle and sqlite support named placeholders, while db2
        /// is lacking this feature.
        bool hasNamedParams = false;

        /// True if this database has user accounts.
        /// Sqlite does not have user accounts.
        bool hasUsers = false;

        /// True if this database driver can report
        /// differences between CHAR and VARCHAR column types.
        /// some databases only have TEXT types, such as sqlite.
        bool hasDistinctTextTypes = false;

        /// Permissions of the current user connection
        Permission permissions = Permission::eNone;
    };

    class IConnection {
        ConnectionInfo mInfo;

    protected:
        IConnection(ConnectionInfo info) noexcept
            : mInfo(std::move(info))
        { }

    public:
        virtual ~IConnection() = default;

        /** Execution */

        virtual IStatement *prepare(std::string_view sql) throws(DbException) = 0;

        /** Transaction */

        virtual DbError begin() noexcept = 0;
        virtual DbError commit() noexcept = 0;
        virtual DbError rollback() noexcept = 0;

        /** Insert */

        virtual std::string setupInsert(const dao::TableInfo& table) throws(DbException) {
            throw DbException{DbError::todoFn()};
        }

        virtual std::string setupInsertOrUpdate(const dao::TableInfo& table) throws(DbException) {
            throw DbException{DbError::todoFn()};
        }

        virtual std::string setupInsertReturningPrimaryKey(const dao::TableInfo& table) throws(DbException) {
            throw DbException{DbError::todoFn()};
        }

        /** Truncate */

        virtual std::string setupTruncate(const dao::TableInfo& table) throws(DbException) {
            throw DbException{DbError::todoFn()};
        }

        /** Select */

        virtual std::string setupSelect(const dao::TableInfo& table) throws(DbException) {
            throw DbException{DbError::todoFn()};
        }

        virtual std::string setupSelectByPrimaryKey(const dao::TableInfo& table) throws(DbException) {
            throw DbException{DbError::todoFn()};
        }

        /** Update */

        virtual std::string setupUpdate(const dao::TableInfo& table) throws(DbException) {
            throw DbException{DbError::todoFn()};
        }

        /** Triggers */

        virtual std::string setupSingletonTrigger(const dao::TableInfo& table) throws(DbException) {
            throw DbException{DbError::todoFn()};
        }

        /** DB specific queries */

        virtual std::string setupTableExists() throws(DbException) {
            throw DbException{DbError::todoFn()};
        }

        virtual std::string setupUserExists() throws(DbException) {
            throw DbException{DbError::todoFn()};
        }

        /** Create */

        virtual std::string setupCreateTable(const dao::TableInfo& table) throws(DbException) {
            throw DbException{DbError::todoFn()};
        }

        virtual std::string setupCommentOnTable(std::string_view table, std::string_view comment) throws(DbException) {
            throw DbException{DbError::todoFn()};
        }

        virtual std::string setupCommentOnColumn(std::string_view table, std::string_view column, std::string_view comment) throws(DbException) {
            throw DbException{DbError::todoFn()};
        }

        /** DB/Client Feature Info */

        Version clientVersion() const noexcept {
            return mInfo.clientVersion;
        }

        Version serverVersion() const noexcept {
            return mInfo.serverVersion;
        }

        DataType boolEquivalentType() const noexcept {
            return mInfo.boolType;
        }

        DataType dateTimeEquivalentType() const noexcept {
            return mInfo.dateTimeType;
        }

        bool hasCommentOn() const noexcept {
            return mInfo.hasCommentOn;
        }

        bool hasNamedParams() const noexcept {
            return mInfo.hasNamedParams;
        }

        bool hasUsers() const noexcept {
            return mInfo.hasUsers;
        }

        bool hasDistinctTextTypes() const noexcept {
            return mInfo.hasDistinctTextTypes;
        }

        Permission getPermissions() const noexcept {
            return mInfo.permissions;
        }
    };

    class IStatement {
    public:
        virtual ~IStatement() = default;

        /** Execution */

        /** Execute statement and fetch first row of data */
        virtual DbError start(bool autoCommit, StatementType type) noexcept = 0;

        /** Complete execution of statement */
        virtual DbError execute() noexcept = 0;

        /** Get next row of data */
        virtual DbError next() noexcept = 0;

        /** Get statements SQL text */
        virtual std::string getSql() const = 0;

        /** Fetch results */

        virtual int getColumnCount() const noexcept {
            return -1;
        }

        virtual int64_t getRowsAffected() const noexcept {
            return -1;
        }

        virtual bool hasDataReady() const noexcept {
            return true;
        }

        virtual DbError getColumnIndex(std::string_view name, int& index) const noexcept {
            return DbError::todo("getColumnIndex");
        }

        virtual DbError getColumnInfo(int index, ColumnInfo& info) const noexcept {
            return DbError::todo("getColumnInfo");
        }

        virtual DbError getColumnInfo(std::string_view name, ColumnInfo& info) const noexcept {
            return DbError::todo("getColumnInfo");
        }

        virtual DbError getIntByIndex(int index, int64_t& value) noexcept {
            return DbError::todo("getIntByIndex");
        }

        virtual DbError getBooleanByIndex(int index, bool& value) noexcept {
            return DbError::todo("getBooleanByIndex");
        }

        virtual DbError getStringByIndex(int index, std::string_view& value) noexcept {
            return DbError::todo("getStringByIndex");
        }

        virtual DbError getDoubleByIndex(int index, double& value) noexcept {
            return DbError::todo("getDoubleByIndex");
        }

        virtual DbError getBlobByIndex(int index, Blob& value) noexcept {
            return DbError::todo("getBlobByIndex");
        }

        virtual DbError getDateTimeByIndex(int index, DateTime& value) noexcept {
            return DbError::todo("getDateTimeByIndex");
        }

        virtual DbError getIntByName(std::string_view column, int64_t& value) noexcept;
        virtual DbError getBooleanByName(std::string_view column, bool& value) noexcept;
        virtual DbError getStringByName(std::string_view column, std::string_view& value) noexcept;
        virtual DbError getDoubleByName(std::string_view column, double& value) noexcept;
        virtual DbError getBlobByName(std::string_view column, Blob& value) noexcept;
        virtual DbError getDateTimeByName(std::string_view column, DateTime& value) noexcept;


        virtual DbError isNullByIndex(int index, bool& value) noexcept {
            return DbError::todo("isNullByName");
        }

        virtual DbError isNullByName(std::string_view column, bool& value) noexcept;

        /** Binding info */
        virtual int getBindCount() const noexcept {
            return -1;
        }

        virtual DbError getBindIndex(std::string_view name, int& index) const noexcept {
            return DbError::todo("getBindIndex");
        }

        /** Binding returning output variables */

        virtual DbError prepareIntReturnByName(std::string_view name) throws(DbException) {
            throw DbException{DbError::todoFn()};
        }

        virtual DbError prepareStringReturnByName(std::string_view name) throws(DbException) {
            throw DbException{DbError::todoFn()};
        }

        /** Fetching returning output variables */

        virtual int64_t getIntReturnByIndex(int index) throws(DbException) {
            throw DbException{DbError::todoFn()};
        }

        virtual std::string_view getStringReturnByIndex(int index) throws(DbException) {
            throw DbException{DbError::todoFn()};
        }

        /** Binding input variables */

        virtual DbError bindIntByIndex(int index, int64_t value) noexcept {
            return DbError::todo("bindIntByIndex");
        }

        virtual DbError bindBooleanByIndex(int index, bool value) noexcept {
            return DbError::todo("bindBooleanByIndex");
        }

        virtual DbError bindStringByIndex(int index, std::string_view value) noexcept {
            return DbError::todo("bindStringByIndex");
        }

        virtual DbError bindDoubleByIndex(int index, double value) noexcept {
            return DbError::todo("bindDoubleByIndex");
        }

        virtual DbError bindBlobByIndex(int index, Blob value) noexcept {
            return DbError::todo("bindBlobByIndex");
        }

        virtual DbError bindDateTimeByIndex(int index, DateTime value) noexcept {
            return DbError::todo("bindDateTimeByIndex");
        }

        virtual DbError bindNullByIndex(int index) noexcept {
            return DbError::todo("bindNullByIndex");
        }

        virtual DbError bindIntByName(std::string_view name, int64_t value) noexcept;
        virtual DbError bindBooleanByName(std::string_view name, bool value) noexcept;
        virtual DbError bindStringByName(std::string_view name, std::string_view value) noexcept;
        virtual DbError bindDoubleByName(std::string_view name, double value) noexcept;
        virtual DbError bindBlobByName(std::string_view name, Blob value) noexcept;
        virtual DbError bindDateTimeByName(std::string_view name, DateTime value) noexcept;
        virtual DbError bindNullByName(std::string_view name) noexcept;

        DbError findColumnIndex(std::string_view name, int& index) const noexcept;
        DbError findBindIndex(std::string_view name, int& index) const noexcept;

    private:
        template<typename T>
        DbError bindValue(std::string_view name, T value, DbError (IStatement::*func)(int, T)) noexcept {
            int index = -1;
            if (DbError error = findBindIndex(name, index))
                return error;

            return (this->*func)(index, value);
        }

        template<typename T>
        DbError getValue(std::string_view name, T& value, DbError (IStatement::*func)(int, T&)) noexcept {
            int index = -1;
            if (DbError error = findColumnIndex(name, index))
                return error;

            return (this->*func)(index, value);
        }
    };

    IEnvironment *newSqliteEnvironment(const EnvConfig& config);
    IEnvironment *newOracleEnvironment(const EnvConfig& config);
    IEnvironment *newPostgresEnvironment(const EnvConfig& config);
    IEnvironment *newDb2Environment();

    // removes any empty lines and trims whitespace
    std::vector<std::string_view> splitComment(std::string_view comment);
}
