#include "stdafx.hpp"
#include "fmt/ranges.h"

#include "orcl.hpp"

#include "drivers/utils.hpp"

using namespace sm::db;

namespace dao = sm::dao;
namespace orcl = sm::db::detail::orcl;
namespace detail = sm::db::detail;

#define STRCASE(ID) case ID: return #ID

std::string orcl::oraErrorText(void *handle, sword status, ub4 type) noexcept {
    auto result = [&] -> std::string {
        text buffer[OCI_ERROR_MAXMSG_SIZE];
        sb4 error;

        switch (status) {
        STRCASE(OCI_SUCCESS);
        STRCASE(OCI_SUCCESS_WITH_INFO);
        STRCASE(OCI_NO_DATA);
        STRCASE(OCI_INVALID_HANDLE);
        STRCASE(OCI_NEED_DATA);
        STRCASE(OCI_STILL_EXECUTING);
        case OCI_ERROR:
            if (sword err = OCIErrorGet(handle, 1, nullptr, &error, buffer, sizeof(buffer), type)) {
                return fmt::format("Failed to get error text: {}", err);
            }
            return std::string{(const char*)(buffer)};

        default:
            return fmt::format("Unknown: {}", status);
        }
    }();

    result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
    std::replace(result.begin(), result.end(), '\n', ' ');

    return result;
}

DbError orcl::oraGetHandleError(void *handle, sword status, ub4 type) noexcept {
    if (status == OCI_SUCCESS)
        return DbError::ok();

    int inner = isSuccess(status) ? DbError::eOk : DbError::eError;
    return DbError{status, inner, oraErrorText(handle, status, type)};
}

bool orcl::isSuccess(sword status) noexcept {
    return status == OCI_SUCCESS || status == OCI_SUCCESS_WITH_INFO;
}

DbError orcl::oraGetError(OraError error, sword status) noexcept {
    return oraGetHandleError(error, status, OCI_HTYPE_ERROR);
}

DbError orcl::oraNewHandle(OCIEnv *env, void **handle, ub4 type) noexcept {
    sword result = OCIHandleAlloc(env, handle, type, 0, nullptr);
    return oraGetHandleError(env, result, OCI_HTYPE_ENV);
}

using sm::dao::ColumnType;

static std::string makeSqlType(const dao::ColumnInfo& info) {
    switch (info.type) {
    case ColumnType::eInt:
    case ColumnType::eLong:
    case ColumnType::eUint:
    case ColumnType::eUlong:
        return "INTEGER";
    case ColumnType::eBool:
        return "BOOLEAN";
    case ColumnType::eFloat:
        return "BINARY_FLOAT";
    case ColumnType::eDouble:
        return "BINARY_DOUBLE";
    case ColumnType::eString:
        return fmt::format("VARCHAR2({})", info.length);
    case ColumnType::eBlob:
        return "BLOB";
    }
}

template<typename F>
static std::vector<std::invoke_result_t<F, size_t, const dao::ColumnInfo&>> forEachField(const dao::TableInfo& info, bool generatePrimaryKey, F&& fn) noexcept {
    std::vector<std::invoke_result_t<F, size_t, const dao::ColumnInfo&>> result;
    size_t primaryKey = detail::primaryKeyIndex(info);
    for (size_t i = 0; i < info.columns.size(); i++) {
        if (generatePrimaryKey && primaryKey == i)
            continue;

        result.push_back(std::invoke(fn, i, info.columns[i]));
    }

    return result;
}

static void setupInsertFields(const dao::TableInfo& info, bool generatePrimaryKey, std::ostream& os) noexcept {
    auto fields = forEachField(info, generatePrimaryKey, [](size_t i, const dao::ColumnInfo& column) {
        return column.name;
    });

    os << fmt::format("{}", fmt::join(fields, ", "));
}

static void setupBindingFields(const dao::TableInfo& info, bool generatePrimaryKey, std::ostream& os) noexcept {
    auto fields = forEachField(info, generatePrimaryKey, [](size_t i, const dao::ColumnInfo& column) {
        return fmt::format(":{}", column.name);
    });

    os << fmt::format("{}", fmt::join(fields, ", "));
}

static void setupInsertCommon(const dao::TableInfo& info, bool generatePrimaryKey, std::ostream& os) noexcept {
    os << "INSERT INTO " << info.name << " (";
    setupInsertFields(info, generatePrimaryKey, os);
    os << ") VALUES (";
    setupBindingFields(info, generatePrimaryKey, os);
    os << ")";
}

std::string orcl::setupInsert(const dao::TableInfo& info) noexcept {
    std::stringstream ss;
    setupInsertCommon(info, false, ss);

    return ss.str();
}

std::string orcl::setupInsertOrUpdate(const dao::TableInfo& info) noexcept {
    std::stringstream ss;

    auto pk = info.getPrimaryKey();

    ss << "MERGE INTO " << info.name << " USING DUAL ON (" << pk.name << " = :" << pk.name << ")";
    ss << " WHEN NOT MATCHED THEN INSERT (";
    setupInsertFields(info, true, ss);
    ss << ") VALUES (";
    setupBindingFields(info, true, ss);
    ss << ") WHEN MATCHED THEN UPDATE SET ";
    auto values = forEachField(info, true, [](size_t i, const dao::ColumnInfo& column) {
        return fmt::format("{0} = :{0}", column.name);
    });

    ss << fmt::format("{}", fmt::join(values, ", "));

    return ss.str();
}

std::string orcl::setupInsertReturningPrimaryKey(const dao::TableInfo& info) noexcept {
    std::stringstream ss;
    setupInsertCommon(info, true, ss);

    auto pk = info.getPrimaryKey();

    ss << " RETURNING " << pk.name << " INTO :" << pk.name;

    return ss.str();
}

std::string orcl::setupCreateTable(const dao::TableInfo& info) noexcept {
    std::ostringstream ss;
    bool hasConstraints = info.hasPrimaryKey() || info.hasForeignKeys();

    ss << "CREATE TABLE";

    ss << " " << info.name << " (\n";
    for (size_t i = 0; i < info.columns.size(); i++) {
        const auto& column = info.columns[i];

        ss << "\t" << column.name << " " << makeSqlType(column) << " NOT NULL";

        if (column.type == dao::ColumnType::eString)
            ss << " CHECK(NOT REGEXP_LIKE(" << column.name << ", '\\s'))";

        if (hasConstraints || (i != info.columns.size() - 1)) {
            ss << ",";
        }

        ss << "\n";
    }

    if (info.hasPrimaryKey()) {
        ss << "\tCONSTRAINT pk_" << info.name
        << " PRIMARY KEY (" << info.getPrimaryKey().name
        << ")";

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

        ss << ")";

        if (i != info.uniqueKeys.size() - 1) {
            ss << ",\n";
        } else {
            ss << "\n";
        }
    }

    ss << ")";

    return ss.str();
}

std::string orcl::setupSelect(const dao::TableInfo& info) noexcept {
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

std::string orcl::setupUpdate(const dao::TableInfo& info) noexcept {
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

static constexpr std::string_view kCreateSingletonTrigger =
    "CREATE TRIGGER IF NOT EXISTS {0}_singleton\n"
    "    BEFORE INSERT ON {0}\n"
    "BEGIN\n"
    "    DELETE FROM {0};\n"
    "END;";

std::string orcl::setupSingletonTrigger(std::string_view name) noexcept {
    return fmt::format(kCreateSingletonTrigger, name);
}

std::string orcl::setupTableExists() noexcept {
    return "SELECT COUNT(*) FROM user_tables WHERE table_name = UPPER(:name)";
}
