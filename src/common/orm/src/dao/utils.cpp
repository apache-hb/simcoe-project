#include "dao/utils.hpp"
#include "core/macros.hpp"
#include <limits.h>

using namespace sm;
using namespace sm::dao;

static std::string_view makeSqlType(ColumnType type) {
    switch (type) {
    case ColumnType::eInt:
    case ColumnType::eLong:
    case ColumnType::eUint:
    case ColumnType::eUlong:
        return "INTEGER";
    case ColumnType::eBool:
        return "BOOLEAN";
    case ColumnType::eFloat:
    case ColumnType::eDouble:
        return "FLOAT";
    case ColumnType::eString:
        return "CHARACTER VARYING(255)";
    case ColumnType::eBlob:
        return "BLOB";
    }
}

void emitRangeCheck(std::ostream& os, std::string_view name, int64_t low, int64_t high) noexcept {
    os << " CHECK(" << name << " > " << low << " AND " << name << " < " << high << ")";
}

db::DbError dao::createTableImpl(db::Connection& connection, const TableInfo& info) noexcept {
    std::ostringstream ss;
    ss << "CREATE TABLE " << info.name << " (\n";
    for (size_t i = 0; i < info.columns.size(); i++) {
        const auto& column = info.columns[i];

        ss << "\t" << column.name << " " << makeSqlType(column.type) << " NOT NULL";

        switch (column.type) {
        case ColumnType::eString:
            ss << " CHECK(NOT IS_BLANK_STRING(" << column.name << "))";
            break;

        case ColumnType::eInt:
            emitRangeCheck(ss, column.name, INT32_MIN, INT32_MAX);
            break;
        case ColumnType::eUint:
            emitRangeCheck(ss, column.name, 0, UINT32_MAX);
            break;
        case ColumnType::eLong:
            emitRangeCheck(ss, column.name, INT64_MIN, INT64_MAX);
            break;
        case ColumnType::eUlong:
            emitRangeCheck(ss, column.name, 0, UINT64_MAX);
            break;

        default: break;
        }

        if (i != info.columns.size() - 1) {
            ss << ",";
        }

        ss << "\n";
    }

    ss << ");";

    return connection.ddlPrepare(ss.str())
        .transform([&](auto stmt) { return stmt.update(); })
        .error_or(db::DbError::ok());
}
