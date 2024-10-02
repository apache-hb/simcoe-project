#pragma once

#include "drivers/common.hpp"

#include "datetime.hpp"
#include "object.hpp"

#include <utility>
#include <variant>

namespace sm::db::detail::orcl {
    class OraStatement;
    class OraConnection;
    class OraEnvironment;

    std::string setupInsert(const dao::TableInfo& info);
    std::string setupInsertOrUpdate(const dao::TableInfo& info);
    std::string setupInsertReturningPrimaryKey(const dao::TableInfo& info);
    std::string setupCreateTable(const dao::TableInfo& info, bool hasBoolType);
    std::string setupSelect(const dao::TableInfo& info);
    std::string setupUpdate(const dao::TableInfo& info);
    std::string setupSingletonTrigger(std::string_view name);
    std::string setupTruncate(std::string_view name);
    std::string setupTableExists();

    DbError oraGetError(OraError error, sword status) noexcept;
    DbError oraNewHandle(OCIEnv *env, void **handle, ub4 type) noexcept;

    template<typename T>
    DbError oraNewHandle(OCIEnv *env, T& handle) noexcept {
        return oraNewHandle(env, handle.voidpp(), T::kType);
    }

    template<typename T>
    using OraResource = sm::UniqueHandle<T, OraDelete<T>>;

    using OraEnvResource = OraResource<OraEnv>;

    template<typename T>
    DbResult<OraResource<T>> oraNewResource(OCIEnv *env, OCIError *error = nullptr) noexcept {
        T handle;
        if (DbError result = oraNewHandle(env, handle))
            return std::unexpected(result);

        OraDelete<T> deleter{error};

        return OraResource<T>{handle, deleter};
    }

    union CellValue {
        char *text;
        OCINumber num;
        OraDateTime date;
        int64 integer;
        double real;
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
        DbError executeUpdate(ub4 flags) noexcept;
        DbError executeSelect(ub4 flags) noexcept;

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

        std::string setupInsert(const dao::TableInfo& table) noexcept(false) override;
        std::string setupInsertOrUpdate(const dao::TableInfo& table) noexcept(false) override;
        std::string setupInsertReturningPrimaryKey(const dao::TableInfo& table) noexcept(false) override;

        std::string setupTruncate(const dao::TableInfo& table) noexcept(false) override;

        std::string setupSelect(const dao::TableInfo& table) noexcept(false) override;

        std::string setupUpdate(const dao::TableInfo& table) noexcept(false) override;

        std::string setupSingletonTrigger(const dao::TableInfo& table) noexcept(false) override;

        std::string setupTableExists() noexcept(false) override;

        std::string setupCreateTable(const dao::TableInfo& table) noexcept(false) override;

        DbError clientVersion(Version& version) const noexcept override;
        DbError serverVersion(Version& version) const noexcept override;

        DataType boolEquivalentType() const noexcept override {
            // if boolean types arent available, we use a string of length 1
            // where '0' is false, and everything else is true (but we prefer '1')
            return hasBoolType() ? DataType::eBoolean : DataType::eString;
        }

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
        bool hasBoolType() const noexcept { return getBoolType() == SQLT_BOL; }

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

        void attach(OraEnv env) noexcept { mEnv = env; }
        OraEnv& env() noexcept { return mEnv; }
    };
}
