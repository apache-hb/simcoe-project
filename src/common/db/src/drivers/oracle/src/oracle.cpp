#include "stdafx.hpp"

#include "oracle/oracle.hpp"

#include "drivers/utils.hpp"

using namespace sm::db;

namespace dao = sm::dao;
namespace oracle = sm::db::oracle;
namespace detail = sm::db::detail;

#define STRCASE(ID) case ID: return #ID

std::string oracle::oraErrorText(void *handle, sword status, ub4 type) noexcept {
    auto result = [&] -> std::string {
        text buffer[OCI_ERROR_MAXMSG_SIZE];
        sb4 error;

        switch (status) {
        STRCASE(OCI_SUCCESS);
        STRCASE(OCI_NO_DATA);
        STRCASE(OCI_INVALID_HANDLE);
        STRCASE(OCI_NEED_DATA);
        STRCASE(OCI_STILL_EXECUTING);

        case OCI_SUCCESS_WITH_INFO: case OCI_ERROR:
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

DbError oracle::oraGetHandleError(void *handle, sword status, ub4 type) noexcept {
    if (status == OCI_SUCCESS)
        return DbError::ok();

    if (status == OCI_NO_DATA)
        return DbError::done(OCI_NO_DATA);

    int inner = isSuccess(status) ? DbError::eOk : DbError::eError;
    return DbError{status, inner, oraErrorText(handle, status, type)};
}

bool oracle::isSuccess(sword status) noexcept {
    return status == OCI_SUCCESS;
}

DbError oracle::oraGetError(OraError error, sword status) noexcept {
    return oraGetHandleError(error, status, OCI_HTYPE_ERROR);
}

DbError oracle::oraNewHandle(OCIEnv *env, void **handle, ub4 type) noexcept {
    sword result = OCIHandleAlloc(env, handle, type, 0, nullptr);
    return oraGetHandleError(env, result, OCI_HTYPE_ENV);
}

static std::string makeSqlType(const dao::ColumnInfo& info, bool hasBoolean) {
    switch (info.type) {
    case dao::ColumnType::eInt:
    case dao::ColumnType::eLong:
    case dao::ColumnType::eUint:
    case dao::ColumnType::eUlong:
        return "INTEGER";
    case dao::ColumnType::eBool:
        return hasBoolean ? "BOOLEAN" : "CHAR(1)";
    case dao::ColumnType::eFloat:
        return "BINARY_FLOAT";
    case dao::ColumnType::eDouble:
        return "BINARY_DOUBLE";
    case dao::ColumnType::eString:
        return fmt::format("VARCHAR2({})", info.length);
    case dao::ColumnType::eBlob:
        return "BLOB";
    case dao::ColumnType::eDateTime:
        return "TIMESTAMP";

    default:
        DbError::todo(fmt::format("Unsupported type {}", toString(info.type))).raise();
    }
}

template<typename F>
static std::vector<std::invoke_result_t<F, size_t, const dao::ColumnInfo&>> forEachField(const dao::TableInfo& info, bool generatePrimaryKey, F&& fn) {
    std::vector<std::invoke_result_t<F, size_t, const dao::ColumnInfo&>> result;
    size_t primaryKey = detail::primaryKeyIndex(info);
    for (size_t i = 0; i < info.columns.size(); i++) {
        if (generatePrimaryKey && primaryKey == i)
            continue;

        result.push_back(std::invoke(fn, i, info.columns[i]));
    }

    return result;
}

static void setupInsertFields(const dao::TableInfo& info, bool generatePrimaryKey, std::ostream& os) {
    auto fields = forEachField(info, generatePrimaryKey, [](size_t i, const dao::ColumnInfo& column) {
        return column.name;
    });

    os << fmt::format("{}", fmt::join(fields, ", "));
}

static void setupBindingFields(const dao::TableInfo& info, bool generatePrimaryKey, std::ostream& os) {
    auto fields = forEachField(info, generatePrimaryKey, [](size_t i, const dao::ColumnInfo& column) {
        return fmt::format(":{}", column.name);
    });

    os << fmt::format("{}", fmt::join(fields, ", "));
}

static std::vector<std::string_view> uniqueColumnNames(const dao::TableInfo& info, const sm::dao::UniqueKey& unique) {
    std::vector<std::string_view> columns;
    columns.reserve(unique.columns.size());
    for (size_t i = 0; i < unique.columns.size(); i++) {
        columns.push_back(info.columns[unique.columns[i]].name);
    }

    return columns;
}

static void setupInsertCommon(const dao::TableInfo& info, bool generatePrimaryKey, std::ostream& os) {
    os << "INSERT INTO " << info.name << " (";
    setupInsertFields(info, generatePrimaryKey, os);
    os << ") VALUES (";
    setupBindingFields(info, generatePrimaryKey, os);
    os << ")";
}

std::string oracle::setupInsert(const dao::TableInfo& info) {
    std::stringstream ss;
    setupInsertCommon(info, false, ss);

    return ss.str();
}

static std::string setupInsertOrReplaceOnPrimaryKey(const dao::TableInfo& info) {
    std::stringstream ss;

    auto pk = info.getPrimaryKey();

    ss << "MERGE INTO " << info.name << " USING DUAL ON (" << pk.name << " = :" << pk.name << ")";
    ss << " WHEN NOT MATCHED THEN INSERT (";
    setupInsertFields(info, false, ss);
    ss << ") VALUES (";
    setupBindingFields(info, false, ss);
    ss << ") WHEN MATCHED THEN UPDATE SET ";
    auto values = forEachField(info, true, [](size_t i, const dao::ColumnInfo& column) {
        return fmt::format("{0} = :{0}", column.name);
    });

    ss << fmt::format("{}", fmt::join(values, ", "));

    return ss.str();
}

static std::string setupInsertOrReplaceOnUniqueKeys(const dao::TableInfo& info) {
    std::unordered_set<std::string_view> uniqueColumns;

    auto createWhereClause = [&](const dao::UniqueKey& unique) {
        auto columns = uniqueColumnNames(info, unique);
        for (auto column : columns) {
            uniqueColumns.insert(column);
        }
        std::vector<std::string> result;
        result.reserve(columns.size());
        for (size_t i = 0; i < columns.size(); i++) {
            result.push_back(fmt::format("{} = :{}", columns[i], columns[i]));
        }

        return fmt::format("({})", fmt::join(result, " AND "));
    };


    std::stringstream ss;

    ss << "MERGE INTO " << info.name << " USING DUAL ON (" << createWhereClause(info.uniqueKeys[0]) << ")";
    ss << " WHEN NOT MATCHED THEN INSERT (";
    setupInsertFields(info, false, ss);
    ss << ") VALUES (";
    setupBindingFields(info, false, ss);
    ss << ")";

    std::vector<std::string> values;
    for (size_t i = 0; i < info.columns.size(); i++) {
        const auto& name = info.columns[i].name;
        if (uniqueColumns.contains(name))
            continue;

        values.push_back(fmt::format("{0} = :{0}", name));
    }

    if (!values.empty()) {
        ss << " WHEN MATCHED THEN UPDATE SET ";
        ss << fmt::format("{}", fmt::join(values, ", "));
    }

    return ss.str();
}

std::string oracle::setupInsertOrUpdate(const dao::TableInfo& info) {
    if (info.hasPrimaryKey()) {
        return setupInsertOrReplaceOnPrimaryKey(info);
    }

    if (!info.uniqueKeys.empty()) {
        return setupInsertOrReplaceOnUniqueKeys(info);
    }

    return setupInsert(info);
}

std::string oracle::setupInsertReturningPrimaryKey(const dao::TableInfo& info) {
    std::stringstream ss;
    setupInsertCommon(info, true, ss);

    auto pk = info.getPrimaryKey();

    ss << " RETURNING " << pk.name << " INTO :" << pk.name;

    return ss.str();
}

std::string oracle::setupCreateTable(const dao::TableInfo& info, bool hasBoolType) {
    std::ostringstream ss;

    ss << "CREATE TABLE";

    ss << " " << info.name << " (\n";
    for (size_t i = 0; i < info.columns.size(); i++) {
        const auto& column = info.columns[i];

        ss << "\t" << column.name << " " << makeSqlType(column, hasBoolType);

        switch (column.autoIncrement) {
        case dao::AutoIncrement::eAlways:
            ss << " GENERATED ALWAYS AS IDENTITY";
            break;
        case dao::AutoIncrement::eDefault:
            ss << " GENERATED BY DEFAULT ON NULL AS IDENTITY";
            break;
        default:
            break;
        }

        if ((i != info.columns.size() - 1)) {
            ss << ",\n";
        }
    }

    std::vector<std::string> constraints;

    auto addConstraint = [&]<typename... A>(fmt::format_string<A...> fmt, A&&... args) {
        constraints.push_back(fmt::format(fmt, std::forward<A>(args)...));
    };

    if (info.hasPrimaryKey()) {
        const auto& pk = info.getPrimaryKey();
        addConstraint("pk_{} PRIMARY KEY({})", info.name, pk.name);
    }

    for (size_t i = 0; i < info.columns.size(); i++) {
        const auto& column = info.columns[i];

        if (column.nullable || column.autoIncrement != dao::AutoIncrement::eNever)
            continue;

        bool isString = column.type == dao::ColumnType::eString;

        addConstraint("ck_{0}_{1}_nonnull CHECK({1} IS NOT NULL)", info.name, column.name);

        if (isString)
            addConstraint("ck_{0}_{1}_nonblank CHECK(REGEXP_LIKE({1}, '\\S'))", info.name, column.name);
    }

    for (size_t i = 0; i < info.foreignKeys.size(); i++) {
        const auto& foreign = info.foreignKeys[i];
        addConstraint("{} FOREIGN KEY({}) REFERENCES {}({})", foreign.name, foreign.column, foreign.foreignTable, foreign.foreignColumn);
    }

    for (size_t i = 0; i < info.uniqueKeys.size(); i++) {
        const auto& unique = info.uniqueKeys[i];

        std::vector<std::string_view> columns = uniqueColumnNames(info, unique);

        addConstraint("{} UNIQUE ({})", unique.name, fmt::join(columns, ", "));
    }

    for (size_t i = 0; i < constraints.size(); i++) {
        ss << ",\n\tCONSTRAINT " << constraints[i];
    }

    ss << "\n)";

    return ss.str();
}

std::string oracle::setupSelect(const dao::TableInfo& info) {
    std::ostringstream ss;
    ss << "SELECT ";
    for (size_t i = 0; i < info.columns.size(); i++) {
        ss << info.columns[i].name;
        if (i != info.columns.size() - 1) {
            ss << ", ";
        }
    }
    ss << " FROM " << info.name;

    return ss.str();
}

std::string oracle::setupUpdate(const dao::TableInfo& info) {
    std::ostringstream ss;
    ss << "UPDATE " << info.name << " SET ";
    for (size_t i = 0; i < info.columns.size(); i++) {
        const auto& name = info.columns[i].name;
        ss << name << " = :" << name;
        if (i != info.columns.size() - 1) {
            ss << ", ";
        }
    }

    return ss.str();
}

static constexpr std::string_view kCreateSingletonTrigger =
    "CREATE TRIGGER IF NOT EXISTS {0}_singleton\n"
    "    BEFORE INSERT ON {0}\n"
    "BEGIN\n"
    "    DELETE FROM {0};\n"
    "END;";

std::string oracle::setupSingletonTrigger(std::string_view name) {
    return fmt::format(kCreateSingletonTrigger, name);
}

std::string oracle::setupTruncate(std::string_view name) {
    return fmt::format("TRUNCATE TABLE {}", name);
}

std::string oracle::setupTableExists() {
    return "SELECT COUNT(*) FROM user_tables WHERE table_name = UPPER(:name)";
}
