#pragma once

#include "drivers/common.hpp"

#include <oci.h>

#include <utility>

namespace sm::db::detail::orcl {
    class OraStatement;
    class OraConnection;
    class OraEnvironment;

#ifdef SQLT_BOL
    static constexpr inline ub2 kBoolType = SQLT_BOL;
#else
    static constexpr inline ub2 kBoolType = SQLT_CHR;
#endif

    static constexpr inline bool kHasBoolType = kBoolType != SQLT_CHR;

    std::string oraErrorText(void *handle, sword status, ub4 type) noexcept;
    DbError oraGetHandleError(void *handle, sword status, ub4 type) noexcept;
    bool isSuccess(sword status) noexcept;

    std::string setupInsert(const dao::TableInfo& info) noexcept;
    std::string setupInsertOrUpdate(const dao::TableInfo& info) noexcept;
    std::string setupInsertReturningPrimaryKey(const dao::TableInfo& info) noexcept;
    std::string setupCreateTable(const dao::TableInfo& info) noexcept;
    std::string setupSelect(const dao::TableInfo& info) noexcept;

    template<typename T, ub4 H>
    class OraHandle final {
        T *mHandle = nullptr;

    public:
        static constexpr inline ub4 kType = H;

        sword setAttribute(OCIError *error, ub4 attr, std::string_view value) const noexcept {
            return OCIAttrSet(mHandle, kType, (void*)value.data(), value.size(), attr, error);
        }

        sword setAttribute(OCIError *error, ub4 attr, const void *value) const noexcept {
            return OCIAttrSet(mHandle, kType, (void*)value, 0, attr, error);
        }

        sword getAttribute(OCIError *error, ub4 attr, void *value, ub4 *size = nullptr) const noexcept {
            return OCIAttrGet(mHandle, kType, value, size, attr, error);
        }

        template<typename A>
        DbResult<A> getAttribute(OCIError *error, ub4 attr) const noexcept {
            A value;
            if (sword status = getAttribute(error, attr, &value))
                return std::unexpected(oraGetHandleError(mHandle, status, OCI_HTYPE_ERROR));

            return value;
        }

        operator T*() const noexcept {
            return mHandle;
        }

        T **operator&() noexcept {
            return &mHandle;
        }

        void **voidpp() noexcept {
            return reinterpret_cast<void**>(&mHandle);
        }

        DbError close(OCIError *error) noexcept {
            sword status = OCIHandleFree(mHandle, kType);
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

    DbError oraGetError(OraError error, sword status) noexcept;
    DbError oraNewHandle(OCIEnv *env, void **handle, ub4 type) noexcept;

    template<typename T>
    DbError oraNewHandle(OCIEnv *env, T& handle) noexcept {
        return oraNewHandle(env, handle.voidpp(), T::kType);
    }

    union CellValue {
        oratext *text;
        OCINumber num;
        OCIDate date;
        int64 integer;
        double real;

        // oratypes.h is rude and defines boolean as a macro
        boolean bol;
    };

    struct OraColumnInfo {
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

    class OraStatement final : public detail::IStatement {
        OraEnvironment& mEnvironment;
        OraConnection& mConnection;
        OraStmt mStatement;
        OraError mError;

        std::vector<OraColumnInfo> mColumnInfo;

        bool useBoolType() const noexcept;

        DbError closeColumns() noexcept;

        DbError executeStatement(ub4 flags, int iters) noexcept;
        DbError executeUpdate(ub4 flags) noexcept;
        DbError executeSelect(ub4 flags) noexcept;

        DbError bindAtPosition(int index, void *value, ub4 size, ub2 type) noexcept;
        DbError bindAtName(std::string_view name, void *value, ub4 size, ub2 type) noexcept;

    public:
        DbError finalize() noexcept override;
        DbError start(bool autoCommit, StatementType type) noexcept override;
        DbError execute() noexcept override;
        DbError next() noexcept override;


        int getBindCount() const noexcept override;

        DbError bindIntByIndex(int index, int64 value) noexcept override;
        DbError bindBooleanByIndex(int index, bool value) noexcept override;
        DbError bindStringByIndex(int index, std::string_view value) noexcept override;
        DbError bindDoubleByIndex(int index, double value) noexcept override;
        DbError bindBlobByIndex(int index, Blob value) noexcept override;
        DbError bindNullByIndex(int index) noexcept override;

        DbError bindIntByName(std::string_view name, int64 value) noexcept override;
        DbError bindBooleanByName(std::string_view name, bool value) noexcept override;
        DbError bindStringByName(std::string_view name, std::string_view value) noexcept override;
        DbError bindDoubleByName(std::string_view name, double value) noexcept override;
        DbError bindBlobByName(std::string_view name, Blob value) noexcept override;
        DbError bindNullByName(std::string_view name) noexcept override;

        DbError update(bool autoCommit) noexcept override;

        int getColumnCount() const noexcept override;
        DbError getColumnIndex(std::string_view name, int& index) const noexcept override;


        DbError getIntByIndex(int index, int64& value) noexcept override;
        DbError getBooleanByIndex(int index, bool& value) noexcept override;
        DbError getStringByIndex(int index, std::string_view& value) noexcept override;
        DbError getDoubleByIndex(int index, double& value) noexcept override;
        DbError getBlobByIndex(int index, Blob& value) noexcept override;

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

    class OraConnection final : public detail::IConnection {
        OraEnvironment& mEnvironment;
        OraError mError;
        OraServer mServer;
        OraService mService;
        OraSession mSession;
        Version mServerVersion;

        DbResult<OraStatement> newStatement(std::string_view sql) noexcept;

        DbError close() noexcept override;

        DbError prepare(std::string_view sql, detail::IStatement **stmt) noexcept override;

        DbError begin() noexcept override;
        DbError commit() noexcept override;
        DbError rollback() noexcept override;

        DbError setupInsert(const dao::TableInfo& table, std::string& sql) noexcept override;
        DbError setupInsertOrUpdate(const dao::TableInfo& table, std::string& sql) noexcept override;
        DbError setupInsertReturningPrimaryKey(const dao::TableInfo& table, std::string& sql) noexcept override;

        DbError setupSelect(const dao::TableInfo& table, std::string& sql) noexcept override;

        DbError createTable(const dao::TableInfo& table) noexcept override;
        DbError tableExists(std::string_view name, bool& exists) noexcept override;

        DbError clientVersion(Version& version) const noexcept override;
        DbError serverVersion(Version& version) const noexcept override;
    public:
        OraConnection(
            OraEnvironment& env, OraError error,
            OraServer server, OraService service,
            OraSession session, Version serverVersion
        ) noexcept
            : mEnvironment(env)
            , mError(error)
            , mServer(server)
            , mService(service)
            , mSession(session)
            , mServerVersion(std::move(serverVersion))
        { }

        ub2 getBoolType() const noexcept;

        OraService& service() noexcept { return mService; }
    };

    class OraEnvironment final : public detail::IEnvironment {
        OraEnv mEnv;

        DbError connect(const ConnectionConfig& config, detail::IConnection **connection) noexcept override;

        bool close() noexcept override {
            return true;
        }

    public:

        void *malloc(size_t size) noexcept;
        void *realloc(void *ptr, size_t size) noexcept;
        void free(void *ptr) noexcept;

        static void *wrapMalloc(void *ctx, size_t size);
        static void *wrapRealloc(void *ctx, void *ptr, size_t size);
        static void wrapFree(void *ctx, void *ptr);

        OraEnvironment() = default;

        void attach(OraEnv env) noexcept {
            mEnv = env;
        }
        OraEnv& env() noexcept { return mEnv; }
    };
}
