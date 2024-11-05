#include "db2/db2.hpp"

using namespace sm::db;

using Db2Environment = sm::db::db2::Db2Environment;
using Db2Connection = sm::db::db2::Db2Connection;
using SqlResult = sm::db::db2::SqlResult;

static constexpr Version kUnknownServerVersion {"IBM DB2", 0, 0, 0, 0, 0};
static constexpr Version kUnknownClientVersion {"IBM DB2 Client", 0, 0, 0, 0, 0};

static std::optional<std::string> getServerInfo(SQLHDBC hdbc, SQLUSMALLINT field) {
    SQLCHAR buffer[255];
    SQLSMALLINT length;
    if (SqlResult result = SQLGetInfo(hdbc, field, buffer, sizeof(buffer), &length)) {
        DbError error = db2::getConnectionErrorInfo(result, hdbc);
        LOG_WARN(DbLog, "Failed to get sql info {}: {}", field, error);
        return std::nullopt;
    }

    return std::string((const char*)buffer, length);
}

static bool hasGetInfoSupport(SQLHDBC hdbc) {
    SQLUSMALLINT supported;
    if (SqlResult result = SQLGetFunctions(hdbc, SQL_API_SQLGETINFO, &supported)) {
        DbError error = db2::getConnectionErrorInfo(result, hdbc);
        LOG_WARN(DbLog, "Failed to get server version: {}", error);
        return false;
    }

    if (supported != SQL_TRUE) {
        LOG_WARN(DbLog, "SQLGetInfo is not supported");
        return false;
    }

    return true;
}

static constexpr size_t kVersionTemplateSize = sizeof("xx.yy.zzzz") - 1;

static bool parseVersion(std::string_view version, int& major, int& minor, int& patch) {
    if (version.size() < kVersionTemplateSize) {
        return false;
    }

    auto parseRange = [&](int front, int back) -> int {
        int value = 0;
        if (auto [ptr, ec] = std::from_chars(version.data() + front, version.data() + back, value); ec != std::errc()) {
            value = 0;
        }

        return value;
    };

    major = parseRange(0, 2);
    minor = parseRange(3, 5);
    patch = parseRange(6, 10);

    return true;
}

static Version getVersionInfo(SQLHDBC hdbc, SQLUSMALLINT name, SQLUSMALLINT version) {
    auto nameInfo = getServerInfo(hdbc, name).value_or("Unknown");
    auto versionInfo = getServerInfo(hdbc, version).value_or("00.00.0000");

    // SQL_DBMS_VER and SQL_DRIVER_VER are in the format 'xx.yy.zzzz'
    // where xx is the major version, yy is the minor version, and zzzz is the release version.
    int major = 0, minor = 0, release = 0;
    if (!parseVersion(versionInfo, major, minor, release)) {
        LOG_WARN(DbLog, "Failed to parse version: {}", versionInfo);
        return kUnknownServerVersion;
    }

    return Version{nameInfo, major, minor, release, 0, 0};
}

static Version getServerVersion(SQLHDBC hdbc) {
    return getVersionInfo(hdbc, SQL_DBMS_NAME, SQL_DBMS_VER);
}

static Version getClientVersion(SQLHDBC hdbc) {
    return getVersionInfo(hdbc, SQL_DRIVER_NAME, SQL_DRIVER_VER);
}

static detail::ConnectionInfo buildConnectionInfo(SQLHDBC hdbc) {
    bool hasGetInfo = hasGetInfoSupport(hdbc);
    Version client = hasGetInfo ? getClientVersion(hdbc) : kUnknownClientVersion;
    Version server = hasGetInfo ? getServerVersion(hdbc) : kUnknownServerVersion;

    return detail::ConnectionInfo {
        .clientVersion = client,
        .serverVersion = server,
        .hasCommentOn = true,
        .hasNamedParams = false,
        .hasUsers = true,
        .hasDistinctTextTypes = true,
    };
}

detail::IStatement *Db2Connection::prepare(std::string_view sql) noexcept(false) {
    SqlStmtHandle hstmt = SqlStmtHandle::create(mDbHandle);

    if (SqlResult result = SQLPrepare(hstmt, (SQLCHAR*)sql.data(), (SQLINTEGER)sql.size()))
        throw DbException{getStmtErrorInfo(result, hstmt)};

    SQLSMALLINT params;
    if (SqlResult result = SQLNumParams(hstmt, &params))
        throw DbException{getStmtErrorInfo(result, hstmt)};

#if 0
    for (SQLSMALLINT i = 1; i <= params; i++) {
        SQLSMALLINT dataType;
        SQLSMALLINT decimalDigits;
        SQLSMALLINT nullable;
        SQLUINTEGER columnSize;

        SQLDescribeParam(hstmt, i, &dataType, &columnSize, &decimalDigits, &nullable);
    }
#endif

    return new Db2Statement(std::move(hstmt), std::string{sql});
}

DbError Db2Connection::setAutoCommit(bool autoCommit) noexcept {
    SQLPOINTER enabled = autoCommit ? (SQLPOINTER)SQL_AUTOCOMMIT_ON : (SQLPOINTER)SQL_AUTOCOMMIT_OFF;
    if (SqlResult result = SQLSetConnectAttr(mDbHandle, SQL_ATTR_AUTOCOMMIT, enabled, 0))
        return getConnectionErrorInfo(result, mDbHandle);

    return DbError::ok();
}

DbError Db2Connection::endTransaction(SQLSMALLINT completionType) noexcept {
    if (SqlResult result = SQLEndTran(SQL_HANDLE_DBC, mDbHandle, completionType))
        return getConnectionErrorInfo(result, mDbHandle);

    return setAutoCommit(true);
}

DbError Db2Connection::begin() noexcept {
    return setAutoCommit(false);
}

DbError Db2Connection::commit() noexcept {
    return endTransaction(SQL_COMMIT);
}

DbError Db2Connection::rollback() noexcept {
    return endTransaction(SQL_ROLLBACK);
}

std::string Db2Connection::setupUserExists() noexcept(false) {
    return "SELECT COUNT(*) FROM SYSIBM.SYSDBAUTH WHERE GRANTEE = UPPER(?)";
}

std::string Db2Connection::setupTableExists() noexcept(false) {
    return "SELECT COUNT(*) FROM SYSIBM.SYSTABLES WHERE TYPE = 'T' AND NAME = UPPER(?)";
}

Db2Connection::~Db2Connection() noexcept {
    if (SqlResult result = SQLDisconnect(mDbHandle)) {
        DbError error = mDbHandle.getErrorInfo(result);
        LOG_WARN(DbLog, "Failed to disconnect: {}", error);
    }
}

Db2Connection::Db2Connection(SqlDbHandle hdbc) noexcept
    : detail::IConnection(buildConnectionInfo(hdbc))
    , mDbHandle(std::move(hdbc))
{ }
