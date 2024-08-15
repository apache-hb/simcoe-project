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

        double getDouble(int index) noexcept;
        int64 getInt(int index) noexcept;
        bool getBool(int index) noexcept;
        std::string_view getString(int index) noexcept;
        Blob getBlob(int index) noexcept;

        double getDouble(std::string_view column) noexcept;
        int64 getInt(std::string_view column) noexcept;
        bool getBool(std::string_view column) noexcept;
        std::string_view getString(std::string_view column) noexcept;
        Blob getBlob(std::string_view column) noexcept;


        template<typename T>
        T get(std::string_view column) noexcept;

        template<std::integral T>
        T get(std::string_view column) noexcept { return static_cast<T>(getInt(column)); }
        template<std::floating_point T>

        T get(std::string_view column) noexcept { return static_cast<T>(getDouble(column)); }

        template<> std::string_view get<std::string_view>(std::string_view column) noexcept { return getString(column); }
        template<> Blob get<Blob>(std::string_view column) noexcept { return getBlob(column); }
        template<> bool get<bool>(std::string_view column) noexcept { return getBool(column); }


        template<typename T>
        T get(int index) noexcept;

        template<std::integral T>
        T get(int index) noexcept { return static_cast<T>(getInt(index)); }

        template<std::floating_point T>
        T get(int index) noexcept { return static_cast<T>(getDouble(index)); }

        template<> std::string_view get<std::string_view>(int index) noexcept { return getString(index); }
        template<> Blob get<Blob>(int index) noexcept { return getBlob(index); }
        template<> bool get<bool>(int index) noexcept { return getBool(index); }
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

        bool mAutoCommit;

        Connection(detail::IConnection *impl, const ConnectionConfig& config) noexcept
            : mImpl(impl)
            , mAutoCommit(config.autoCommit)
        { }

    public:
        SM_MOVE(Connection, default);

        void begin();
        void rollback();
        void commit();

        void setAutoCommit(bool enabled) noexcept { mAutoCommit = enabled; }
        bool autoCommit() const noexcept { return mAutoCommit; }

        std::expected<PreparedStatement, DbError> prepare(std::string_view sql);

        std::expected<ResultSet, DbError> select(std::string_view sql);
        std::expected<ResultSet, DbError> update(std::string_view sql);

        bool tableExists(std::string_view name);
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

    std::string_view toString(DbType type) noexcept;
}
