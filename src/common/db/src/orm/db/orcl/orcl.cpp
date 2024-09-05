#include "stdafx.hpp"

#include "db/orcl/orcl.hpp"

using namespace sm;
using namespace sm::db;

namespace orcl = sm::db::detail::orcl;

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
