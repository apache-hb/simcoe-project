#pragma once

#include "drivers/common.hpp"

#include "types/datetime.hpp"
#include "types/object.hpp"

#include <utility>
#include <variant>

namespace sm::db::oracle {
    class OraStatement;
    class OraConnection;
    class OraEnvironment;

    union CellValue {
        char *text;
        OCINumber num;
        OraDateTime date;
        int64 integer;
        double bdouble;
        void *lob;

        // oratypes.h is rude and defines boolean as a macro
        boolean bol;
    };

    struct OraColumnInfo {
        std::string_view name;
        ub4 charSemantics;
        ub4 columnWidth;
        OraDefine define{nullptr};

        CellValue value;
        ub2 type;
        ub4 valueLength;
        sb2 precision;
        sb1 scale;
        sb2 indicator;
    };

    using BindValue = std::variant<std::string, OraDateTime, int64_t, double, Blob>;

    class OraStatement final : public detail::IStatement {
        OraEnvironment& mEnvironment;
        OraConnection& mConnection;
        OraStmt mStatement;
        OraError mError;
        bool mHasData = false;

        std::forward_list<BindValue> mBindValues;
        std::vector<OraColumnInfo> mColumnInfo;

        template<typename T>
        auto addBindValue(T value) {
            auto& result = mBindValues.emplace_front(std::move(value));
            return &std::get<T>(result);
        }

        bool hasDataReady() const noexcept override { return mHasData; }

        DbError closeColumns() noexcept;
        void freeBindValues() noexcept;

        DbError executeStatement(ub4 flags, int iters) noexcept;

        DbError bindAtPosition(int index, void *value, ub4 size, ub2 type) noexcept;
        DbError bindAtName(std::string_view name, void *value, ub4 size, ub2 type) noexcept;

        DbError initDateTime(DateTime initial, OraDateTime &result) noexcept;
        DbError extractDateTime(const OraDateTime& datetime, DateTime& result) noexcept;

    public:
        DbError finalize() noexcept override;
        DbError start(bool autoCommit, StatementType type) noexcept override;
        DbError execute() noexcept override;
        DbError next() noexcept override;


        int getBindCount() const noexcept override;
        DbError getBindIndex(std::string_view name, int& index) const noexcept override;

        DbError bindIntByIndex(int index, int64 value) noexcept override;
        DbError bindBooleanByIndex(int index, bool value) noexcept override;
        DbError bindStringByIndex(int index, std::string_view value) noexcept override;
        DbError bindDoubleByIndex(int index, double value) noexcept override;
        DbError bindBlobByIndex(int index, Blob value) noexcept override;
        DbError bindDateTimeByIndex(int index, DateTime value) noexcept override;
        DbError bindNullByIndex(int index) noexcept override;

        DbError bindIntByName(std::string_view name, int64 value) noexcept override;
        DbError bindBooleanByName(std::string_view name, bool value) noexcept override;
        DbError bindStringByName(std::string_view name, std::string_view value) noexcept override;
        DbError bindDoubleByName(std::string_view name, double value) noexcept override;
        DbError bindBlobByName(std::string_view name, Blob value) noexcept override;
        DbError bindDateTimeByName(std::string_view name, DateTime value) noexcept override;
        DbError bindNullByName(std::string_view name) noexcept override;

        DbError isNullByIndex(int index, bool& value) noexcept override;
        DbError isNullByName(std::string_view column, bool& value) noexcept override;

        int getColumnCount() const noexcept override;
        DbError getColumnIndex(std::string_view name, int& index) const noexcept override;

        DbError getColumnInfo(int index, ColumnInfo& info) const noexcept override;
        DbError getColumnInfo(std::string_view name, ColumnInfo& info) const noexcept override;

        DbError getIntByIndex(int index, int64& value) noexcept override;
        DbError getBooleanByIndex(int index, bool& value) noexcept override;
        DbError getStringByIndex(int index, std::string_view& value) noexcept override;
        DbError getDoubleByIndex(int index, double& value) noexcept override;
        DbError getBlobByIndex(int index, Blob& value) noexcept override;
        DbError getDateTimeByIndex(int index, DateTime& value) noexcept override;

        OraStatement(
            OraEnvironment& environment,
            OraConnection& connection,
            OraStmt stmt, OraError error
        ) noexcept
            : mEnvironment(environment)
            , mConnection(connection)
            , mStatement(stmt)
            , mError(error)
        { }
    };

}
