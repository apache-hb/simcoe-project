#pragma once

#include "core/memory/unique.hpp"
#include "orm/error.hpp"

#include "core/core.hpp"
#include "core/macros.hpp"

#include <expected>
#include <span>

namespace sm::db {
    class ResultSet;
    class QueryIterator;
    class Query;
    class PreparedStatement;
    class Connection;
    class Environment;

    using Blob = std::span<const uint8>;
    using Date = std::chrono::time_point<std::chrono::system_clock>;

    namespace detail {
        struct IStatement;
        struct IConnection;
        struct IEnvironment;

        void destroyStatement(detail::IStatement *impl) noexcept;
        void destroyConnection(detail::IConnection *impl) noexcept;
        void destroyEnvironment(detail::IEnvironment *impl) noexcept;

        using StmtHandle = std::shared_ptr<IStatement>;
        using ConnectionHandle = FnUniquePtr<detail::IConnection, &destroyConnection>;
        using EnvHandle = FnUniquePtr<detail::IEnvironment, &destroyEnvironment>;
    }

    enum class DbType {
        eSqlite3,
        ePostgreSQL,
        eMySQL,
        eOracleDB,
        eMSSQL,
        eDB2
    };

    enum class DataType {
        eInteger,
        eBoolean,
        eString,
        eDouble,
        eBlob,
        eNull,
        eRowId
    };

    struct ConnectionConfig {
        uint16 port;
        std::string_view host;
        std::string_view user;
        std::string_view password;
        std::string_view database;

        int timeout = 5;
    };

    struct Column {
        std::string_view name;
        DataType type;
    };

    /// @brief Represents a result set of a query.
    ///
    /// @note Not internally synchronized.
    /// @note Resetting the statement that produced the result set will invalidate the result set.
    /// @note Columns are 0-indexed.
    class ResultSet {
        friend PreparedStatement;

        detail::StmtHandle mImpl;
        detail::IEnvironment *mEnv = nullptr;

        ResultSet(detail::StmtHandle impl, detail::IEnvironment *env) noexcept
            : mImpl(std::move(impl))
            , mEnv(env)
        { }

    public:
        DbError next() noexcept;

        int columnCount() const noexcept;
        int columnIndex(std::string_view name) const noexcept;

        Column column(int index) const noexcept;

        double getDouble(int index) noexcept;
        int64 getInt(int index) noexcept;
        bool getBool(int index) noexcept;
        std::string_view getString(int index) noexcept;
        Blob getBlob(int index) noexcept;

        double getDouble(std::string_view column) noexcept { return getDouble(columnIndex(column)); }
        int64 getInt(std::string_view column) noexcept { return getInt(columnIndex(column)); }
        bool getBool(std::string_view column) noexcept { return getBool(columnIndex(column)); }
        std::string_view getString(std::string_view column) noexcept { return getString(columnIndex(column)); }
        Blob getBlob(std::string_view column) noexcept { return getBlob(columnIndex(column)); }
    };

    class BindPoint {
        friend PreparedStatement;

        detail::IStatement *mImpl = nullptr;
        int mIndex = 0;

        BindPoint(detail::IStatement *impl, int index) noexcept
            : mImpl(impl)
            , mIndex(index)
        { }

    public:
        void to(int64 value) noexcept;
        void to(bool value) noexcept;
        void to(std::string_view value) noexcept;
        void to(double value) noexcept;
        void to(Blob value) noexcept;
        void to(std::nullptr_t) noexcept;

        BindPoint& operator=(int64 value) noexcept { to(value); return *this; }
        BindPoint& operator=(bool value) noexcept { to(value); return *this; }
        BindPoint& operator=(std::string_view value) noexcept { to(value); return *this; }
        BindPoint& operator=(double value) noexcept { to(value); return *this; }
        BindPoint& operator=(Blob value) noexcept { to(value); return *this; }
        BindPoint& operator=(std::nullptr_t) noexcept { to(nullptr); return *this; }
    };

    class PreparedStatement {
        friend Connection;

        detail::StmtHandle mImpl;
        detail::IEnvironment *mEnv = nullptr;

        PreparedStatement(detail::IStatement *impl, detail::IEnvironment *env) noexcept
            : mImpl({impl, &detail::destroyStatement})
            , mEnv(env)
        { }

    public:
        SM_MOVE(PreparedStatement, default);

        BindPoint bind(int index) noexcept { return BindPoint{mImpl.get(), index}; }
        void bind(int index, int64 value) noexcept { bind(index) = value; }
        void bind(int index, std::string_view value) noexcept { bind(index) = value; }
        void bind(int index, double value) noexcept { bind(index) = value; }
        void bind(int index, std::nullptr_t) noexcept { bind(index) = nullptr; }

        BindPoint bind(std::string_view name) noexcept;
        void bind(std::string_view name, int64 value) noexcept { bind(name) = value; }
        void bind(std::string_view name, std::string_view value) noexcept { bind(name) = value; }
        void bind(std::string_view name, double value) noexcept { bind(name) = value; }
        void bind(std::string_view name, std::nullptr_t) noexcept { bind(name) = nullptr; }

        std::expected<ResultSet, DbError> select() noexcept;
        std::expected<ResultSet, DbError> update() noexcept;

        DbError close() noexcept;
        DbError reset() noexcept;
    };

    class Connection {
        friend Environment;

        detail::ConnectionHandle mImpl;
        detail::IEnvironment *mEnv = nullptr;

        Connection(detail::IConnection *impl, detail::IEnvironment *env) noexcept
            : mImpl(impl)
            , mEnv(env)
        { }

    public:
        SM_MOVE(Connection, default);

        DbError begin() noexcept;
        DbError rollback() noexcept;
        DbError commit() noexcept;

        std::expected<PreparedStatement, DbError> prepare(std::string_view sql) noexcept;

        std::expected<ResultSet, DbError> select(std::string_view sql) noexcept;
        std::expected<ResultSet, DbError> update(std::string_view sql) noexcept;

        bool tableExists(std::string_view name) noexcept;
    };

    class Environment {
        detail::EnvHandle mImpl;

        Environment(detail::IEnvironment *impl) noexcept
            : mImpl(impl)
        { }

    public:
        SM_MOVE(Environment, default);

        static bool isSupported(DbType type) noexcept;
        static std::expected<Environment, DbError> create(DbType type) noexcept;

        std::expected<Connection, DbError> connect(const ConnectionConfig& config) noexcept;
    };
}
