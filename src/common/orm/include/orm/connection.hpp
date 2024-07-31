#pragma once

#include "orm/bind.hpp"
#include "orm/core.hpp"
#include "orm/error.hpp"

#include "core/core.hpp"
#include "core/macros.hpp"

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

    class PreparedStatement {
        friend Connection;

        detail::StmtHandle mImpl;
        Connection *mConnection;

        PreparedStatement(detail::IStatement *impl, Connection *connection) noexcept
            : mImpl({impl, &detail::destroyStatement})
            , mConnection(connection)
        { }

    public:
        SM_MOVE(PreparedStatement, default);

        PositionalBindPoint bind(int index) noexcept { return PositionalBindPoint{mImpl.get(), index}; }
        void bind(int index, int64 value) noexcept { bind(index) = value; }
        void bind(int index, std::string_view value) noexcept { bind(index) = value; }
        void bind(int index, double value) noexcept { bind(index) = value; }
        void bind(int index, std::nullptr_t) noexcept { bind(index) = nullptr; }

        NamedBindPoint bind(std::string_view name) noexcept;
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

        bool mAutoCommit;

        Connection(detail::IConnection *impl, const ConnectionConfig& config) noexcept
            : mImpl(impl)
            , mAutoCommit(config.autoCommit)
        { }

    public:
        SM_MOVE(Connection, default);

        DbError begin() noexcept;
        DbError rollback() noexcept;
        DbError commit() noexcept;

        void setAutoCommit(bool enabled) noexcept { mAutoCommit = enabled; }
        bool autoCommit() const noexcept { return mAutoCommit; }

        std::expected<PreparedStatement, DbError> prepare(std::string_view sql) noexcept;

        std::expected<ResultSet, DbError> select(std::string_view sql) noexcept;
        std::expected<ResultSet, DbError> update(std::string_view sql) noexcept;

        bool tableExists(std::string_view name) noexcept;
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

        static bool isSupported(DbType type) noexcept;
        static std::expected<Environment, DbError> create(DbType type) noexcept;

        std::expected<Connection, DbError> connect(const ConnectionConfig& config) noexcept;
    };
}
