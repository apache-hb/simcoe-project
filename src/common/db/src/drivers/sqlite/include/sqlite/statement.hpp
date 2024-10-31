#pragma once

#include <sqlite3.h>

#include "drivers/common.hpp"

namespace sm::db::sqlite {
    class SqliteStatement final : public detail::IStatement {
        sqlite3_stmt *mStatement = nullptr;
        int mStatus = SQLITE_OK;

        DbError getStmtError(int err) const noexcept;
        bool hasDataReady() const noexcept override { return mStatus == SQLITE_ROW; }

    public:
        DbError finalize() noexcept override;
        DbError start(bool autoCommit, StatementType type) noexcept override;
        DbError execute() noexcept override;
        DbError next() noexcept override;
        std::string getSql() const override;

        int getBindCount() const noexcept override;

        DbError getBindIndex(std::string_view name, int& index) const noexcept override;

        DbError bindIntByIndex(int index, int64 value) noexcept override;
        DbError bindBooleanByIndex(int index, bool value) noexcept override;
        DbError bindStringByIndex(int index, std::string_view value) noexcept override;
        DbError bindDoubleByIndex(int index, double value) noexcept override;
        DbError bindBlobByIndex(int index, Blob value) noexcept override;
        DbError bindDateTimeByIndex(int index, DateTime value) noexcept override;
        DbError bindNullByIndex(int index) noexcept override;

        int getColumnCount() const noexcept override;
        DbError getColumnIndex(std::string_view name, int& index) const noexcept override;

        DbError getColumnInfo(int index, ColumnInfo& info) const noexcept override;
        DbError getColumnInfo(std::string_view name, ColumnInfo& info) const noexcept override;

        /** No-ops on sqlite */
        DbError prepareIntReturnByName(std::string_view name) noexcept(false) override { return DbError::ok(); }
        DbError prepareStringReturnByName(std::string_view name) noexcept(false) override { return DbError::ok(); }

        int64_t getIntReturnByIndex(int index) noexcept(false) override {
            int64_t value = -1;
            getIntByIndex(index, value).throwIfFailed();
            return value;
        }

        std::string_view getStringReturnByIndex(int index) noexcept(false) override {
            std::string_view value;
            getStringByIndex(index, value).throwIfFailed();
            return value;
        }

        DbError getIntByIndex(int index, int64& value) noexcept override;
        DbError getBooleanByIndex(int index, bool& value) noexcept override;
        DbError getStringByIndex(int index, std::string_view& value) noexcept override;
        DbError getDoubleByIndex(int index, double& value) noexcept override;
        DbError getDateTimeByIndex(int index, DateTime& value) noexcept override;
        DbError getBlobByIndex(int index, Blob& value) noexcept override;

        DbError isNullByIndex(int index, bool& value) noexcept override;
        DbError isNullByName(std::string_view column, bool& value) noexcept override;

        SqliteStatement(sqlite3_stmt *stmt) noexcept;
    };
}
