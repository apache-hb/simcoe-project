#pragma once

#include "db/bind.hpp"
#include "db/error.hpp"

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

    void bindRowToStatement(PreparedStatement& stmt, const dao::TableInfo& info, bool returning, const void *data) noexcept;
}
