#include "dao/utils.hpp"
#include "core/macros.hpp"

#include <fmtlib/format.h>

#include <limits.h>

using namespace sm;
using namespace sm::dao;

static std::string makeSqlType(const ColumnInfo& info) {
    switch (info.type) {
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
        return fmt::format("CHARACTER VARYING({})", info.length);
    case ColumnType::eBlob:
        return "BLOB";
    }
}

static void emitRangeCheck(std::ostream& os, std::string_view name, int64_t low, int64_t high) noexcept {
    os << " CHECK(" << name << " >= " << low << " AND " << name << " <= " << high << ")";
}

static std::string_view onConflictString(OnConflict conflict) noexcept {
    switch (conflict) {
    case OnConflict::eRollback:
        return "ROLLBACK";
    case OnConflict::eAbort:
        return "ABORT";
    case OnConflict::eIgnore:
        return "IGNORE";
    case OnConflict::eReplace:
        return "REPLACE";
    }

    CT_NEVER("Invalid on conflict value");
}

static std::string setupCreateTable(const TableInfo& info, CreateTable options) noexcept {
    std::ostringstream ss;
    bool hasConstraints = info.hasPrimaryKey() || info.hasForeignKeys();

    ss << "CREATE TABLE";

    if (options == CreateTable::eCreateIfNotExists)
        ss << " IF NOT EXISTS";

    ss << " " << info.name << " (\n";
    for (size_t i = 0; i < info.columns.size(); i++) {
        const auto& column = info.columns[i];

        ss << "\t" << column.name << " " << makeSqlType(column) << " NOT NULL";

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

        if (hasConstraints || (i != info.columns.size() - 1)) {
            ss << ",";
        }

        ss << "\n";
    }

    if (info.hasPrimaryKey()) {
        ss << "\tCONSTRAINT pk_" << info.name
           << " PRIMARY KEY (" << info.getPrimaryKey().name
           << ") ON CONFLICT " << onConflictString(info.onConflict);

        if (info.hasForeignKeys() || info.hasUniqueKeys()) {
            ss << ",\n";
        } else {
            ss << "\n";
        }
    }

    for (size_t i = 0; i < info.foreignKeys.size(); i++) {
        const auto& foreign = info.foreignKeys[i];
        ss << "\tCONSTRAINT " << foreign.name << " FOREIGN KEY (" << foreign.column << ") REFERENCES " << foreign.foreignTable << "(" << foreign.foreignColumn << ")";

        if (info.hasUniqueKeys() || i != info.foreignKeys.size() - 1) {
            ss << ",";
        }

        ss << "\n";
    }

    for (size_t i = 0; i < info.uniqueKeys.size(); i++) {
        const auto& unique = info.uniqueKeys[i];
        ss << "\tCONSTRAINT " << unique.name << " UNIQUE (";
        for (size_t j = 0; j < unique.columns.size(); j++) {
            const auto& column = info.columns[unique.columns[j]];
            ss << column.name;
            if (j != unique.columns.size() - 1) {
                ss << ", ";
            }
        }

        ss << ") ON CONFLICT " << onConflictString(unique.onConflict);

        if (i != info.uniqueKeys.size() - 1) {
            ss << ",\n";
        } else {
            ss << "\n";
        }
    }

    ss << ");";

    return ss.str();
}

db::DbError dao::createTableImpl(db::Connection& connection, const TableInfo& info, CreateTable options) noexcept {
    auto sql = setupCreateTable(info, options);

    fmt::println(stderr, "Creating table: {}", sql);

    return connection.ddlPrepare(sql)
        .and_then([](auto stmt) { return stmt.update(); })
        .error_or(db::DbError::ok());
}

static std::string setupInsert(const TableInfo& info, bool returning) noexcept {
    std::ostringstream ss;
    ss << "INSERT INTO " << info.name << " (";
    for (size_t i = 0; i < info.columns.size(); i++) {
        if (returning && info.primaryKey == i)
            continue;

        ss << info.columns[i].name;
        if (i != info.columns.size() - 1) {
            ss << ", ";
        }
    }

    ss << ") VALUES (";
    for (size_t i = 0; i < info.columns.size(); i++) {
        if (returning && info.primaryKey == i)
            continue;

        ss << ":" << i;
        if (i != info.columns.size() - 1) {
            ss << ", ";
        }
    }

    ss << ")";

    if (returning) {
        auto pk = info.getPrimaryKey();
        ss << " RETURNING " << pk.name;
    }

    ss << ";";

    return ss.str();
}

static void bindIndex(db::PreparedStatement& stmt, const TableInfo& info, size_t index, bool returning, const void *data) noexcept {
    const auto& column = info.columns[index];
    if (returning && info.primaryKey == index)
        return;

    auto i = fmt::format("{}", index);
    const void *field = static_cast<const char*>(data) + column.offset;
    switch (column.type) {
    case ColumnType::eInt:
        stmt.bind(i).toInt(*reinterpret_cast<const int32_t*>(field));
        break;
    case ColumnType::eUint:
        stmt.bind(i).toInt(*reinterpret_cast<const uint32_t*>(field));
        break;
    case ColumnType::eLong:
        stmt.bind(i).toInt(*reinterpret_cast<const int64_t*>(field));
        break;
    case ColumnType::eUlong:
        stmt.bind(i).toInt(*reinterpret_cast<const uint64_t*>(field));
        break;
    case ColumnType::eBool:
        stmt.bind(i).toBool(*reinterpret_cast<const bool*>(field));
        break;
    case ColumnType::eString:
        stmt.bind(i).toString(*reinterpret_cast<const std::string*>(field));
        break;
    case ColumnType::eFloat:
        stmt.bind(i).toDouble(*reinterpret_cast<const float*>(field));
        break;
    case ColumnType::eDouble:
        stmt.bind(i).toDouble(*reinterpret_cast<const double*>(field));
        break;
    default:
        CT_NEVER("Unsupported column type");
    }
}

db::DbError dao::insertImpl(db::Connection& connection, const TableInfo& info, const void *data) noexcept {
    auto sql = setupInsert(info, false);

    return connection.dmlPrepare(sql)
        .and_then([&](auto stmt) {
            for (size_t i = 0; i < info.columns.size(); i++) {
                bindIndex(stmt, info, i, false, data);
            }

            return stmt.update();
        })
        .error_or(db::DbError::ok());
}

db::DbError dao::insertGetPrimaryKeyImpl(db::Connection& connection, const TableInfo& info, const void *data, void *generated) noexcept {
    CTASSERTF(info.hasAutoIncrementPrimaryKey(), "Table %s has no auto increment primary key", info.name.data());

    auto sql = setupInsert(info, true);

    auto result = connection.dmlPrepare(sql)
        .and_then([&](auto stmt) {
            for (size_t i = 0; i < info.columns.size(); i++) {
                bindIndex(stmt, info, i, true, data);
            }

            return stmt.update();
        });

    auto value = TRY_UNWRAP(result);

    auto pk = info.getPrimaryKey();
    switch (pk.type) {
    case ColumnType::eInt:
        *reinterpret_cast<int32_t*>(generated) = value.getInt(0);
        break;
    case ColumnType::eUint:
        *reinterpret_cast<uint32_t*>(generated) = value.getInt(0);
        break;
    case ColumnType::eLong:
        *reinterpret_cast<int64_t*>(generated) = value.getInt(0);
        break;
    case ColumnType::eUlong:
        *reinterpret_cast<uint64_t*>(generated) = value.getInt(0);
        break;
    default:
        CT_NEVER("Unsupported primary key type");
    }

    return db::DbError::ok();
}

const ColumnInfo& TableInfo::getPrimaryKey() const noexcept {
    CTASSERTF(hasPrimaryKey(), "Table %s has no primary key", name.data());
    return columns[primaryKey];
}
