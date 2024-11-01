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

        OraStatement newStatement(std::string_view sql) throws(DbException);

        DbError close() noexcept override;

        detail::IStatement *prepare(std::string_view sql) noexcept(false) override;

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
        std::string setupUserExists() noexcept(false) override;

        std::string setupCreateTable(const dao::TableInfo& table) noexcept(false) override;

        std::string setupCommentOnTable(std::string_view table, std::string_view comment) noexcept(false) override;

        std::string setupCommentOnColumn(std::string_view table, std::string_view column, std::string_view comment) noexcept(false) override;

    public:
        OraConnection(
            OraEnvironment& env, OraError error,
            OraServer server, OraService service,
            OraSession session
        );

        ub2 getBoolType() const noexcept;
        bool hasBoolType() const noexcept { return getBoolType() == SQLT_BOL; }

        OraService& service() noexcept { return mService; }
    };
}
