#pragma once

#include "drivers/common.hpp"

#include "datetime.hpp"
#include "object.hpp"

#include "statement.hpp"

namespace sm::db::oracle {
    class OraStatement;
    class OraConnection;
    class OraEnvironment;

    std::string setupInsert(const dao::TableInfo& info);
    std::string setupInsertOrUpdate(const dao::TableInfo& info);
    std::string setupInsertReturningPrimaryKey(const dao::TableInfo& info);
    std::string setupCreateTable(const dao::TableInfo& info, bool hasBoolType);
    std::string setupCommentOnTable(std::string_view name, std::string_view comment);
    std::string setupCommentOnColumn(std::string_view table, std::string_view column, std::string_view comment);
    std::string setupSelect(const dao::TableInfo& info);
    std::string setupSelectByPrimaryKey(const dao::TableInfo& info);
    std::string setupUpdate(const dao::TableInfo& info);
    std::string setupSingletonTrigger(std::string_view name);

    DbError oraGetError(OraError error, sword status);
    DbError oraNewHandle(OCIEnv *env, void **handle, ub4 type);

    template<typename T>
    DbError oraNewHandle(OCIEnv *env, T& handle) noexcept {
        return oraNewHandle(env, handle.voidpp(), T::kType);
    }

    template<typename T>
    using OraResource = sm::UniqueHandle<T, OraDelete<T>>;

    using OraEnvResource = OraResource<OraEnv>;

    template<typename T>
    OraResource<T> oraNewResource(OCIEnv *env, OCIError *error = nullptr) noexcept {
        T handle;
        oraNewHandle(env, handle).throwIfFailed();

        OraDelete<T> deleter{error};

        return OraResource<T>{handle, deleter};
    }
}
