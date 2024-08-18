#include "stdafx.hpp"

#include "orm/error.hpp"

using namespace sm;
using namespace sm::db;

DbError::DbError(int code, int status, std::string message) noexcept
    : mCode(code)
    , mStatus(status)
    , mMessage(std::move(message))
{
    if (!isSuccess())
        mStacktrace = std::stacktrace::current();
}

void DbError::raise() const noexcept(false) {
    if (mStatus == eConnectionError)
        throw DbConnectionException{mCode, mMessage};

    throw DbException{mCode, mMessage};
}

DbError DbError::ok() noexcept {
    return DbError{0, eOk, "OK"};
}

DbError DbError::todo() noexcept {
    return DbError{-1, eUnimplemented, "Not implemented"};
}

DbError DbError::error(int code, std::string message) noexcept {
    return DbError{code, eError, std::move(message)};
}

DbError DbError::noMoreData(int code) noexcept {
    return DbError{code, eNoMoreData, "No more data"};
}

DbError DbError::unsupported(std::string_view subject) noexcept {
    return DbError{-1, eError, fmt::format("Unsupported {}", subject)};
}

DbError DbError::columnNotFound(std::string_view column) noexcept {
    return DbError{-1, eError, fmt::format("Column not found: {}", column)};
}

DbError DbError::bindNotFound(std::string_view bind) noexcept {
    return DbError{-1, eError, fmt::format("Bind not found: {}", bind)};
}

DbError DbError::connectionError(std::string_view message) noexcept {
    return DbError{-1, eConnectionError, std::string{message}};
}
