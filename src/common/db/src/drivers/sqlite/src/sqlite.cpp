#include "stdafx.hpp"

#include "sqlite/sqlite.hpp"

using namespace sm;
using namespace sm::db;

static int getStatusType(int err) {
    switch (err) {
    case SQLITE_OK:
    case SQLITE_ROW:
        return DbError::eOk;

    case SQLITE_DONE:
        return DbError::eDone;

    default:
        return DbError::eError;
    }
}

DbError sqlite::getError(int err) noexcept {
    int status = getStatusType(err);

    return DbError{err, status, sqlite3_errstr(err)};
}

DbError sqlite::getError(int err, sqlite3 *db, const char *message) noexcept {
    if (err == SQLITE_OK || err == SQLITE_ROW)
        return DbError::ok();

    if (err == SQLITE_DONE)
        return DbError::done(err);

    int status = getStatusType(err);
    const char *msg = message != nullptr ? message : sqlite3_errmsg(db);
    return DbError{err, status, msg};
}

using sm::dao::ColumnType;

static std::string_view makeSqlType(const dao::ColumnInfo& info) noexcept {
    switch (info.type) {
    case ColumnType::eInt:
    case ColumnType::eLong:
    case ColumnType::eUint:
    case ColumnType::eUlong:
    case ColumnType::eBool:
    case ColumnType::eDateTime:
        return "INTEGER";
    case ColumnType::eFloat:
    case ColumnType::eDouble:
        return "REAL";
    case ColumnType::eChar: case ColumnType::eVarChar:
        return "TEXT";
    case ColumnType::eBlob:
        return "BLOB";

    default:
        CT_NEVER("Invalid column type: %d", (int)info.type);
    }
}

static void sqlTableComment(std::ostream& os, std::string_view comment) {
    for (std::string_view part : detail::splitComment(comment)) {
        os << "-- " << part << "\n";
    }
}

static void sqlColumnComment(std::ostream& os, std::string_view comment) {
    for (std::string_view part : detail::splitComment(comment)) {
        os << "\t-- " << part << "\n";
    }
}

static bool isWhitespace(std::string_view str) noexcept {
    for (char c : str) {
        if (!std::isspace(c))
            return false;
    }

    return true;
}

std::string sqlite::setupCreateTable(const dao::TableInfo& info) noexcept {
    std::ostringstream ss;
    bool hasConstraints = info.hasPrimaryKey() || info.hasForeignKeys();

    ss << "CREATE TABLE " << info.name;

    if (!isWhitespace(info.comment)) {
        ss << "\n";
        sqlTableComment(ss, info.comment);
        ss << "(\n";
    } else {
        ss << " (\n";
    }

    for (size_t i = 0; i < info.columns.size(); i++) {
        const auto& column = info.columns[i];

        sqlColumnComment(ss, column.comment);
        ss << "\t" << column.name << " " << makeSqlType(column);
        if (!column.nullable)
            ss << " NOT NULL";

        if (isStringType(column.type) && !column.nullable)
            ss << " CHECK(NOT IS_BLANK_STRING(" << column.name << "))";

        if (hasConstraints || (i != info.columns.size() - 1)) {
            ss << ",";
        }

        ss << "\n";
    }

    if (info.hasPrimaryKey()) {
        const auto& pk = info.getPrimaryKey();
        ss << "\tCONSTRAINT pk_" << info.name
           << " PRIMARY KEY (" << pk.name << ")";

        if (info.hasForeignKeys() || info.hasUniqueKeys()) {
            ss << ",\n";
        } else {
            ss << "\n";
        }
    }

    for (size_t i = 0; i < info.foreignKeys.size(); i++) {
        const auto& foreign = info.foreignKeys[i];
        ss << "\tCONSTRAINT " << foreign.name
           << " FOREIGN KEY (" << foreign.column << ") REFERENCES "
           << foreign.foreignTable << "(" << foreign.foreignColumn << ")";

        switch (foreign.onDelete) {
        case dao::OnDelete::eCascade:
            ss << " ON DELETE CASCADE";
            break;
        case dao::OnDelete::eSetNull:
            ss << " ON DELETE SET NULL";
            break;
        default:
            break;
        }

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

        ss << ")";

        if (i != info.uniqueKeys.size() - 1) {
            ss << ",\n";
        } else {
            ss << "\n";
        }
    }

    ss << ") STRICT;";

    return ss.str();
}

static constexpr std::string_view kCreateSingletonTrigger =
    "CREATE TRIGGER IF NOT EXISTS {0}_singleton\n"
    "BEFORE INSERT ON {0}\n"
    "WHEN (SELECT COUNT(*) FROM {0}) > 0\n"
    "BEGIN\n"
    "    DELETE FROM {0};\n"
    "END;";

std::string sqlite::setupCreateSingletonTrigger(std::string_view name) noexcept {
    return fmt::format(kCreateSingletonTrigger, name);
}

static size_t primaryKeyIndex(const dao::TableInfo& info) noexcept {
    if (!info.hasPrimaryKey())
        return SIZE_MAX;

    return std::distance(info.columns.data(), info.primaryKey);
}

static void setupInsertCommon(const dao::TableInfo& info, bool generatePrimaryKey, std::ostream& os) noexcept {
    size_t primaryKey = primaryKeyIndex(info);

    os << "INSERT INTO " << info.name << " (";
    for (size_t i = 0; i < info.columns.size(); i++) {
        if (generatePrimaryKey && primaryKey == i)
            continue;

        os << info.columns[i].name;
        if (i != info.columns.size() - 1) {
            os << ", ";
        }
    }

    os << ") VALUES (";
    for (size_t i = 0; i < info.columns.size(); i++) {
        if (generatePrimaryKey && primaryKey == i)
            continue;

        os << ":" << info.columns[i].name;
        if (i != info.columns.size() - 1) {
            os << ", ";
        }
    }

    os << ")";
}

std::string sqlite::setupInsert(const dao::TableInfo& info) noexcept {
    std::stringstream ss;
    setupInsertCommon(info, false, ss);
    ss << ";";

    return ss.str();
}

std::string sqlite::setupInsertOrUpdate(const dao::TableInfo& info) noexcept {
    std::ostringstream ss;
    setupInsertCommon(info, false, ss);
    ss << " ON CONFLICT DO UPDATE SET ";
    for (size_t i = 0; i < info.columns.size(); i++) {
        ss << info.columns[i].name << " = :" << info.columns[i].name;
        if (i != info.columns.size() - 1) {
            ss << ", ";
        }
    }

    ss << ";";

    return ss.str();
}

std::string sqlite::setupInsertReturningPrimaryKey(const dao::TableInfo& info) noexcept {
    std::ostringstream ss;
    setupInsertCommon(info, true, ss);

    auto pk = info.getPrimaryKey();
    ss << " ON CONFLICT DO UPDATE SET "
        << pk.name << "=" << pk.name
        << " RETURNING " << pk.name << ";";

    return ss.str();
}

std::string sqlite::setupUpdate(const dao::TableInfo& info) noexcept {
    std::ostringstream ss;
    ss << "UPDATE " << info.name << " SET ";
    for (size_t i = 0; i < info.columns.size(); i++) {
        ss << info.columns[i].name << " = :" << info.columns[i].name;
        if (i != info.columns.size() - 1) {
            ss << ", ";
        }
    }

    ss << ";";

    return ss.str();
}

std::string sqlite::setupSelect(const dao::TableInfo& info) noexcept {
    std::ostringstream ss;
    ss << "SELECT ";
    for (size_t i = 0; i < info.columns.size(); i++) {
        ss << info.columns[i].name;
        if (i != info.columns.size() - 1) {
            ss << ", ";
        }
    }
    ss << " FROM " << info.name << ";";

    return ss.str();
}
