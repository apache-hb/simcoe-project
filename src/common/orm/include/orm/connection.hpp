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
        Column column(int index) const noexcept;

        double getDouble(int index) throws(DbException);
        int64 getInt(int index) throws(DbException);
        bool getBool(int index) throws(DbException);
        std::string_view getString(int index) throws(DbException);
        Blob getBlob(int index) throws(DbException);

        double getDouble(std::string_view column) throws(DbException);
        int64 getInt(std::string_view column) throws(DbException);
        bool getBool(std::string_view column) throws(DbException);
        std::string_view getString(std::string_view column) throws(DbException);
        Blob getBlob(std::string_view column) throws(DbException);

        template<typename T>
        T get(std::string_view column) throws(DbException);

        template<typename T>
        T get(int index) throws(DbException);

        template<std::integral T>
        T get(int column) throws(DbException) { return static_cast<T>(getInt(column)); }

        template<std::floating_point T>
        T get(int column) throws(DbException) { return static_cast<T>(getDouble(column)); }

        template<std::integral T>
        T get(std::string_view column) throws(DbException) { return static_cast<T>(getInt(column)); }

        template<std::floating_point T>
        T get(std::string_view column) throws(DbException) { return static_cast<T>(getDouble(column)); }
    };

#define RESULT_SET_GET_IMPL(type, method) \
    template<> \
    inline type ResultSet::get<type>(std::string_view column) throws(DbException) { \
        return method(column); \
    } \
    template<> \
    inline type ResultSet::get<type>(int index) throws(DbException) { \
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

        PreparedStatement(detail::IStatement *impl, Connection *connection) noexcept
            : mImpl({impl, &detail::destroyStatement})
            , mConnection(connection)
        { }

    public:
        SM_MOVE(PreparedStatement, default);

        BindPoint bind(std::string_view name) throws(DbException);
        void bind(std::string_view name, int64 value) throws(DbException) { bind(name) = value; }
        void bind(std::string_view name, std::string_view value) throws(DbException) { bind(name) = value; }
        void bind(std::string_view name, double value) throws(DbException) { bind(name) = value; }
        void bind(std::string_view name, std::nullptr_t) throws(DbException) { bind(name) = nullptr; }

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

        void begin() throws(DbException);
        void rollback() throws(DbException);
        void commit() throws(DbException);

        void setAutoCommit(bool enabled) noexcept { mAutoCommit = enabled; }

        [[nodiscard]]
        bool autoCommit() const noexcept { return mAutoCommit; }

        std::expected<PreparedStatement, DbError> prepare(std::string_view sql) noexcept;

        std::expected<ResultSet, DbError> select(std::string_view sql) noexcept;
        std::expected<ResultSet, DbError> update(std::string_view sql) noexcept;

        [[nodiscard]]
        bool tableExists(std::string_view name) throws(DbException);

        std::expected<Version, DbError> dbVersion() const noexcept;
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
        static std::expected<Environment, DbError> create(DbType type) noexcept;

        std::expected<Connection, DbError> connect(const ConnectionConfig& config) noexcept;
    };

    std::string_view toString(DbType type) noexcept;
}
