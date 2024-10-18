#pragma once

#include "drivers/common.hpp"

#include "oracle.hpp"

namespace sm::db::oracle {
    class OraConnection final : public detail::IConnection {
        OraEnvironment& mEnvironment;
        OraError mError;
        OraServer mServer;
        OraService mService;
        OraSession mSession;
        Version mServerVersion;

        DbResult<OraStatement> newStatement(std::string_view sql) noexcept;

        DbError close() noexcept override;

        DbError prepare(std::string_view sql, detail::IStatement **stmt) noexcept override;

        DbError begin() noexcept override;
        DbError commit() noexcept override;
        DbError rollback() noexcept override;

        std::string setupInsert(const dao::TableInfo& table) noexcept(false) override;
        std::string setupInsertOrUpdate(const dao::TableInfo& table) noexcept(false) override;
        std::string setupInsertReturningPrimaryKey(const dao::TableInfo& table) noexcept(false) override;

        std::string setupTruncate(const dao::TableInfo& table) noexcept(false) override;

        std::string setupSelect(const dao::TableInfo& table) noexcept(false) override;

        std::string setupUpdate(const dao::TableInfo& table) noexcept(false) override;

        std::string setupSingletonTrigger(const dao::TableInfo& table) noexcept(false) override;

        std::string setupTableExists() noexcept(false) override;

        std::string setupCreateTable(const dao::TableInfo& table) noexcept(false) override;

        std::string setupCommentOnTable(std::string_view table, std::string_view comment) noexcept(false) override;

        std::string setupCommentOnColumn(std::string_view table, std::string_view column, std::string_view comment) noexcept(false) override;

        Version clientVersion() const noexcept override;
        Version serverVersion() const noexcept override;

        DataType boolEquivalentType() const noexcept override {
            // if boolean types arent available, we use a string of length 1
            // where '0' is false, and everything else is true (but we prefer '1')
            return hasBoolType() ? DataType::eBoolean : DataType::eString;
        }

        bool hasCommentOn() const noexcept override {
            return true;
        }

    public:
        OraConnection(
            OraEnvironment& env, OraError error,
            OraServer server, OraService service,
            OraSession session, Version serverVersion
        ) noexcept
            : mEnvironment(env)
            , mError(error)
            , mServer(server)
            , mService(service)
            , mSession(session)
            , mServerVersion(std::move(serverVersion))
        { }

        ub2 getBoolType() const noexcept;
        bool hasBoolType() const noexcept { return getBoolType() == SQLT_BOL; }

        OraService& service() noexcept { return mService; }
    };
}
