#pragma once

#include "drivers/common.hpp"

#include "types/datetime.hpp"
#include "types/object.hpp"

#include "statement.hpp"

namespace sm::db::oracle {
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
}
