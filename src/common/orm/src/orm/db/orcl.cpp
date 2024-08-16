#include "stdafx.hpp"

#include "core/memory/unique.hpp"

#include "common.hpp"

#include <oci.h>

using namespace sm;
using namespace sm::db;

using namespace std::string_literals;
using namespace std::string_view_literals;

#define STRCASE(ID) case ID: return #ID##sv
#define ERRCASE(ID) case ID: return #ID##s

namespace {
    std::string oraErrorText(void *handle, sword status, ub4 type) noexcept {
        std::string result = [&] {
            text buffer[OCI_ERROR_MAXMSG_SIZE];
            sb4 error;

            switch (status) {
            ERRCASE(OCI_SUCCESS);
            ERRCASE(OCI_SUCCESS_WITH_INFO);
            ERRCASE(OCI_NO_DATA);
            ERRCASE(OCI_INVALID_HANDLE);
            ERRCASE(OCI_NEED_DATA);
            ERRCASE(OCI_STILL_EXECUTING);
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

    bool isSuccess(sword status) noexcept {
        return status == OCI_SUCCESS || status == OCI_SUCCESS_WITH_INFO;
    }

    DbError oraGetHandleError(void *handle, sword status, ub4 type) noexcept {
        if (status == OCI_SUCCESS)
            return DbError::ok();

        int inner = isSuccess(status) ? DbError::eOk : DbError::eError;
        return DbError{status, inner, oraErrorText(handle, status, type)};
    }

    template<typename T, ub4 H>
    struct OraHandle {
        static constexpr inline ub4 kType = H;
        T *handle = nullptr;

        sword setAttribute(OCIError *error, ub4 attr, std::string_view value) const noexcept {
            return OCIAttrSet(handle, kType, (void*)value.data(), value.size(), attr, error);
        }

        sword setAttribute(OCIError *error, ub4 attr, const void *value) const noexcept {
            return OCIAttrSet(handle, kType, (void*)value, 0, attr, error);
        }

        sword getAttribute(OCIError *error, ub4 attr, void *value, ub4 *size = nullptr) const noexcept {
            return OCIAttrGet(handle, kType, value, size, attr, error);
        }

        template<typename A>
        std::expected<A, DbError> getAttribute(OCIError *error, ub4 attr) const noexcept {
            A value;
            if (sword status = getAttribute(error, attr, &value))
                return std::unexpected(oraGetHandleError(handle, status, OCI_HTYPE_ERROR));

            return value;
        }

        operator T*() const noexcept {
            return handle;
        }

        T **operator&() noexcept {
            return &handle;
        }

        void **voidpp() noexcept {
            return reinterpret_cast<void**>(&handle);
        }

        DbError close(OCIError *error) noexcept {
            sword status = OCIHandleFree(handle, kType);
            return oraGetHandleError(error, status, OCI_HTYPE_ERROR);
        }
    };

    using OraError = OraHandle<OCIError, OCI_HTYPE_ERROR>;
    using OraEnv = OraHandle<OCIEnv, OCI_HTYPE_ENV>;
    using OraServer = OraHandle<OCIServer, OCI_HTYPE_SERVER>;
    using OraService = OraHandle<OCISvcCtx, OCI_HTYPE_SVCCTX>;
    using OraSession = OraHandle<OCISession, OCI_HTYPE_SESSION>;
    using OraTsx = OraHandle<OCITrans, OCI_HTYPE_TRANS>;
    using OraStmt = OraHandle<OCIStmt, OCI_HTYPE_STMT>;
    using OraBind = OraHandle<OCIBind, OCI_HTYPE_BIND>;
    using OraDefine = OraHandle<OCIDefine, OCI_HTYPE_DEFINE>;

    using OraParam = OraHandle<OCIParam, OCI_DTYPE_PARAM>;

    DbError oraGetError(OraError error, sword status) noexcept {
        return oraGetHandleError(error, status, OCI_HTYPE_ERROR);
    }

    DbError oraNewHandle(OCIEnv *env, void **handle, ub4 type) noexcept {
        sword result = OCIHandleAlloc(env, handle, type, 0, nullptr);
        return oraGetHandleError(env, result, OCI_HTYPE_ENV);
    }

    template<typename T>
    DbError oraNewHandle(OCIEnv *env, T& handle) noexcept {
        return oraNewHandle(env, handle.voidpp(), T::kType);
    }

    union CellValue {
        oratext *text;
        char num[21];
        OCIDate date;
        int64 integer;
        double real;
    };

    struct ColumnInfo {
        std::string_view name;
        ub4 charSemantics;
        ub4 columnWidth;
        OraDefine define;

        CellValue value;
        ub2 type;
        ub2 valueLength;
        sb2 precision;
        sb1 scale;
    };

    struct CellInfo {
        void *value;
        ub4 length;
    };

    bool isStringType(ub2 type) noexcept {
        return type == SQLT_STR || type == SQLT_AFC || type == SQLT_CHR;
    }

    bool isNumberType(ub2 type) noexcept {
        return type == SQLT_NUM || type == SQLT_INT || type == SQLT_FLT;
    }

    CellInfo initCellValue(OraError error, ColumnInfo& column) noexcept {
        if (isStringType(column.type)) {
            column.value.text = (oratext*)std::malloc(column.columnWidth + 1);
            return CellInfo { column.value.text, column.columnWidth + 1 };
        }

        switch (column.type) {
        case SQLT_NUM:
            return CellInfo { &column.value.num, sizeof(column.value.num) };

        case SQLT_DAT:
            return CellInfo { &column.value.date, sizeof(column.value.date) };

        case SQLT_INT:
            return CellInfo { &column.value.integer, sizeof(column.value.integer) };

        case SQLT_FLT:
            return CellInfo { &column.value.real, sizeof(column.value.real) };

        default:
            CT_NEVER("Unsupported data type %d", column.type);
        }
    }

    std::expected<ub2, DbError> getColumnSize(OraError error, OraParam param, bool charSemantics) noexcept {
        if (charSemantics) {
            return param.getAttribute<ub2>(error, OCI_ATTR_CHAR_SIZE);
        } else {
            return param.getAttribute<ub2>(error, OCI_ATTR_DATA_SIZE);
        }
    }

    std::expected<std::string_view, DbError> getColumnName(OraError error, OraParam param) noexcept {
        ub4 columnNameLength = 0;
        text *columnNameBuffer = nullptr;
        if (sword status = param.getAttribute(error, OCI_ATTR_NAME, &columnNameBuffer, &columnNameLength))
            return std::unexpected(oraGetError(error, status));

        return std::string_view{(const char*)columnNameBuffer, columnNameLength};
    }

    std::expected<std::vector<ColumnInfo>, DbError> defineColumns(OraError error, OraStmt stmt) noexcept {
        OraParam param;
        std::vector<ColumnInfo> columns;
        ub4 counter = 1;
        sword result = OCIParamGet(stmt, OCI_HTYPE_STMT, error, param.voidpp(), counter);
        if (result != OCI_SUCCESS)
            return columns;

        columns.reserve(16);

        while (result == OCI_SUCCESS) {
            ub2 dataType = TRY_RESULT(param.getAttribute<ub2>(error, OCI_ATTR_DATA_TYPE));

            // TODO: this is definetly going to bite me in the future
            if (dataType == SQLT_NUM)
                dataType = SQLT_INT;

            std::string_view columnName = TRY_RESULT(getColumnName(error, param));

            ub1 charSemantics = TRY_RESULT(param.getAttribute<ub1>(error, OCI_ATTR_CHAR_USED));

            ub2 columnWidth = TRY_RESULT(getColumnSize(error, param, charSemantics));

            auto& info = columns.emplace_back(ColumnInfo {
                .name = columnName,
                .charSemantics = charSemantics,
                .columnWidth = columnWidth,
                .type = dataType,
            });

            if (isNumberType(dataType)) {
                info.precision = TRY_RESULT(param.getAttribute<sb2>(error, OCI_ATTR_PRECISION));
                info.scale = TRY_RESULT(param.getAttribute<sb1>(error, OCI_ATTR_SCALE));
            }

            auto [value, size] = initCellValue(error, info);

            sword status = OCIDefineByPos(
                stmt, &info.define, error, counter,
                value, size, dataType,
                nullptr, &info.valueLength, nullptr,
                OCI_DEFAULT
            );

            if (status != OCI_SUCCESS)
                return std::unexpected(oraGetError(error, status));

            result = OCIParamGet(stmt, OCI_HTYPE_STMT, error, param.voidpp(), ++counter);
        }

        return columns;
    }
}

class OraStatement final : public detail::IStatement {
    OraService& mService;
    OraStmt mStmt;
    OraError mError;

    std::vector<ColumnInfo> mColumnInfo;
    sword mStatus = OCI_SUCCESS;

    DbError closeColumns() noexcept {
        for (ColumnInfo& column : mColumnInfo) {
            if (isStringType(column.type))
                std::free(column.value.text);
        }

        mColumnInfo.clear();
        return DbError::ok();
    }

    DbError execute(ub4 flags, int iters) noexcept {
        if (DbError error = closeColumns())
            return error;

        mStatus = OCIStmtExecute(mService, mStmt, mError, iters, 0, nullptr, nullptr, flags);

        bool shouldDefine = isSuccess(mStatus);
        if (mStatus == OCI_NO_DATA)
            mStatus = OCI_SUCCESS;

        if (!isSuccess(mStatus))
            return oraGetError(mError, mStatus);

        if (shouldDefine)
            mColumnInfo = TRY_UNWRAP(defineColumns(mError, mStmt));

        return DbError::ok();
    }

    DbError executeUpdate(ub4 flags) noexcept {
        return execute(flags, 1);
    }

    DbError executeSelect(ub4 flags) noexcept {
        return execute(flags, 0);
    }

    DbError close() noexcept override {
        if (DbError error = closeColumns())
            return error;

        if (sword result = OCIStmtRelease(mStmt, mError, nullptr, 0, OCI_DEFAULT))
            return oraGetError(mError, result);

        DbError result = mError.close(nullptr);
        return oraGetError(mError, result);
    }

    DbError bindInt(int index, int64 value) noexcept override {
        return DbError::todo();
    }

    DbError bindBoolean(int index, bool value) noexcept override {
        return DbError::todo();
    }

    DbError bindString(int index, std::string_view value) noexcept override {
        OraBind bind;
        sword status = OCIBindByPos(
            mStmt, &bind, mError, index + 1,
            (void*)value.data(), value.size(), SQLT_CHR,
            nullptr, nullptr, nullptr,
            0, nullptr, OCI_DEFAULT
        );

        if (status != OCI_SUCCESS)
            return oraGetError(mError, status);

        return DbError::ok();
    }

    DbError bindDouble(int index, double value) noexcept override {
        return DbError::todo();
    }

    DbError bindBlob(int index, Blob value) noexcept override {
        return DbError::todo();
    }

    DbError bindNull(int index) noexcept override {
        return DbError::todo();
    }

    DbError select() noexcept override {
        return executeSelect(OCI_DEFAULT);
    }

    DbError update(bool autoCommit) noexcept override {
        ub4 flags = autoCommit ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT;
        return executeUpdate(flags);
    }

    DbError reset() noexcept override {
        // TODO: some statements need to be reset, others dont
        return DbError::todo();
    }

    int columnCount() const noexcept override {
        return mColumnInfo.size();
    }

    DbError next() noexcept override {
        sword result = OCIStmtFetch2(mStmt, mError, 1, OCI_FETCH_NEXT, 0, OCI_DEFAULT);
        return oraGetError(mError, result);
    }

    DbError getInt(int index, int64& value) noexcept override {
        const ColumnInfo& column = mColumnInfo[index];
        const CellValue& cell = column.value;

        if (column.type == SQLT_NUM) {
            if ((column.scale == 0) || (column.precision == 0)) {
                auto [_, ec] = std::from_chars(cell.num, cell.num + column.valueLength, value);
                if (ec != std::errc{}) {
                    return DbError::error((int)ec, "Failed to parse integer value");
                }
            } else {
                return DbError::error(OCI_ERROR, "Column is not an integer");
            }
        } else {
            value = cell.integer;
        }

        return DbError::ok();
    }

    DbError getBoolean(int index, bool& value) noexcept override {
        value = mColumnInfo[index].value.text[0] != '0';
        return DbError::ok();
    }

    DbError getString(int index, std::string_view& value) noexcept override {
        const char *text = (const char*)mColumnInfo[index].value.text;
        value = {text, mColumnInfo[index].valueLength};
        return DbError::ok();
    }

    DbError getDouble(int index, double& value) noexcept override {
        value = mColumnInfo[index].value.real;
        return DbError::ok();
    }

    DbError getBlob(int index, Blob& value) noexcept override {
        return DbError::todo();
    }

public:
    OraStatement(OraService& service, OraStmt stmt, OraError error) noexcept
        : mService(service)
        , mStmt(stmt)
        , mError(error)
    { }
};

class OraConnection final : public detail::IConnection {
    OraEnv& mEnv;
    OraError mError;
    OraServer mServer;
    OraService mService;
    OraSession mSession;

    DbError close() noexcept override {
        if (sword result = OCISessionEnd(mService, mError, mSession, OCI_DEFAULT))
            return oraGetError(mError, result);

        if (sword result = OCIServerDetach(mServer, mError, OCI_DEFAULT))
            return oraGetError(mError, result);

        if (DbError result = mSession.close(mError))
            return result;

        if (DbError result = mService.close(mError))
            return result;

        if (DbError result = mServer.close(mError))
            return result;

        if (DbError result = mError.close(nullptr))
            return result;

        return DbError::ok();
    }

    DbError prepare(std::string_view sql, detail::IStatement **stmt) noexcept override {
        OraStmt statement;
        OraError error;
        if (DbError result = oraNewHandle(mEnv, error))
            return result;

        if (DbError result = oraNewHandle(mEnv, statement))
            return result;

        if (sword result = OCIStmtPrepare2(mService, &statement, error, (text*)sql.data(), sql.size(), nullptr, 0, OCI_NTV_SYNTAX, OCI_DEFAULT))
            return oraGetError(error, result);

        *stmt = new OraStatement{mService, statement, error};

        return DbError::ok();
    }

    DbError begin() noexcept override {
        // not needed on Oracle
        return DbError::ok();
    }

    DbError commit() noexcept override {
        sword result = OCITransCommit(mService, mError, OCI_DEFAULT);
        return oraGetError(mError, result);
    }

    DbError rollback() noexcept override {
        sword result = OCITransRollback(mService, mError, OCI_DEFAULT);
        return oraGetError(mError, result);
    }

    DbError tableExists(std::string_view name, bool& exists) noexcept override {
        detail::IStatement *stmt = nullptr;
        if (DbError result = prepare("SELECT COUNT(*) FROM user_tables WHERE table_name = UPPER(:1)", &stmt))
            return result;

        stmt->bindString(0, name);

        if (DbError result = stmt->select())
            return result;

        if (DbError result = stmt->next())
            return result;

        int64 count;
        if (DbError result = stmt->getInt(0, count))
            return result;

        exists = count > 0;

        return stmt->close();
    }

    DbError dbVersion(Version& version) const noexcept override {
        text buffer[512];
        ub4 release;
        if (sword error = OCIServerRelease2(mServer, mError, buffer, sizeof(buffer), OCI_HTYPE_SERVER, &release, OCI_DEFAULT))
            return oraGetError(mError, error);

        int major = OCI_SERVER_RELEASE_REL(release);
        int minor = OCI_SERVER_RELEASE_REL_UPD(release);
        int patch = OCI_SERVER_RELEASE_REL_UPD_REV(release);

        version = Version{(const char*)buffer, major, minor, patch};

        return DbError::ok();
    }

public:
    OraConnection(OraEnv& env, OraError error, OraServer server, OraService service, OraSession session) noexcept
        : mEnv(env)
        , mError(error)
        , mServer(server)
        , mService(service)
        , mSession(session)
    { }
};

static constexpr std::string_view kConnectionString = R"(
    (DESCRIPTION=
        (CONNECT_TIMEOUT={})
        (ADDRESS=(PROTOCOL=TCP)(HOST={})(PORT={}))
        (CONNECT_DATA=(SERVICE_NAME={}))
    )
)";

class OraEnvironment final : public detail::IEnvironment {
    OraEnv mEnv;

    DbError connect(const ConnectionConfig& config, detail::IConnection **connection) noexcept override {
        OraServer server;
        OraError error;
        OraService service;
        OraSession session;

        if (DbError result = oraNewHandle(mEnv, error))
            return result;

        if (DbError result = oraNewHandle(mEnv, server))
            return result;

        /** Attach to the server */
        std::string conn = fmt::format(kConnectionString, config.timeout.count(), config.host, config.port, config.database);
        if (sword result = OCIServerAttach(server, error, (text*)conn.data(), conn.size(), OCI_DEFAULT))
            return oraGetError(error, result);

        if (DbError result = oraNewHandle(mEnv, service))
            return result;

        if (sword result = service.setAttribute(error, OCI_ATTR_SERVER, server))
            return oraGetError(error, result);

        if (DbError result = oraNewHandle(mEnv, session))
            return result;

        if (sword result = session.setAttribute(error, OCI_ATTR_USERNAME, config.user))
            return oraGetError(error, result);

        if (sword result = session.setAttribute(error, OCI_ATTR_PASSWORD, config.password))
            return oraGetError(error, result);

        if (sword result = OCISessionBegin(service, error, session, OCI_CRED_RDBMS, OCI_STMT_CACHE))
            return oraGetError(error, result);

        if (sword result = service.setAttribute(error, OCI_ATTR_SESSION, session))
            return oraGetError(error, result);

        *connection = new OraConnection{mEnv, error, server, service, session};

        return DbError::ok();
    }

    void *malloc(size_t size) noexcept {
        void *ptr = std::malloc(size);
        return ptr;
    }

    void *realloc(void *ptr, size_t size) noexcept {
        void *result = std::realloc(ptr, size);
        return result;
    }

    void free(void *ptr) noexcept {
        std::free(ptr);
    }

    bool close() noexcept override {
        return true;
    }

public:
    static void *wrapMalloc(void *ctx, size_t size) {
        auto env = static_cast<OraEnvironment*>(ctx);
        return env->malloc(size);
    }

    static void *wrapRealloc(void *ctx, void *ptr, size_t size) {
        auto env = static_cast<OraEnvironment*>(ctx);
        return env->realloc(ptr, size);
    }

    static void wrapFree(void *ctx, void *ptr) {
        auto env = static_cast<OraEnvironment*>(ctx);
        env->free(ptr);
    }

    OraEnvironment() = default;

    void attach(OraEnv env) noexcept {
        mEnv = env;
    }
};

DbError detail::oracledb(IEnvironment **env) noexcept {
    UniquePtr orcl = makeUnique<OraEnvironment>();
    OraEnv oci;
    sword result = OCIEnvCreate(
        &oci.handle, OCI_THREADED, orcl.get(),
        &OraEnvironment::wrapMalloc,
        &OraEnvironment::wrapRealloc,
        &OraEnvironment::wrapFree,
        0, nullptr
    );

    if (result != OCI_SUCCESS)
        return oraGetHandleError(oci, result, OCI_HTYPE_ENV);

    orcl->attach(oci);

    *env = orcl.release();

    return DbError::ok();
}
