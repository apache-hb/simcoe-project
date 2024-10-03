#include "stdafx.hpp"

#include "db/error.hpp"

using namespace sm;
using namespace sm::db;

DbError::DbError(int code, int status, std::string message, bool enableStackTrace) noexcept
    : mCode(code)
    , mStatus(status)
    , mMessage(std::move(message))
{
    if (!isSuccess() && enableStackTrace) {
        mStacktrace = std::stacktrace::current();

        LOG_WARN(DbLog, "DbError {}: {}", mMessage, mCode);
    }
}

void DbError::raise() const noexcept(false) {
    throw DbException{*this};
}

void DbError::throwIfFailed() const noexcept(false) {
    if (!isSuccess())
        raise();
}

DbError DbError::ok() noexcept {
    return DbError{0, eOk, "OK"};
}

DbError DbError::todo() noexcept {
    return DbError{-1, eUnimplemented, "Not implemented"};
}

DbError DbError::todo(std::string_view subject) noexcept {
    return DbError{-1, eUnimplemented, fmt::format("Not implemented: {}", subject)};
}

DbError DbError::error(int code, std::string message) noexcept {
    return DbError{code, eError, std::move(message)};
}

DbError DbError::outOfMemory() noexcept {
    return DbError{-1, eError, "Out of memory"};
}

DbError DbError::noData() noexcept {
    return DbError{-1, eNoData, "No data"};
}

DbError DbError::columnIsNull(int index) noexcept {
    return DbError{-1, eError, fmt::format("Column `{}` is null", index)};
}

DbError DbError::columnIsNull(std::string_view name) noexcept {
    return DbError{-1, eError, fmt::format("Column `{}` is null", name)};
}

DbError DbError::done(int code) noexcept {
    return DbError{code, eDone, "Done", false};
}

DbError DbError::unsupported(std::string_view subject) noexcept {
    return DbError{-1, eError, fmt::format("Unsupported {}", subject)};
}

DbError DbError::columnNotFound(std::string_view column) noexcept {
    return DbError{-1, eError, fmt::format("Column not found: `{}`", column)};
}

DbError DbError::typeMismatch(std::string_view column, DataType actual, DataType expected) noexcept {
    return DbError{-1, eError, fmt::format("Column `{}` type mismatch: expected {}, column was {}", column, expected, actual)};
}

DbError DbError::bindNotFound(std::string_view bind) noexcept {
    return DbError{-1, eError, fmt::format("Named bind `{}` not found", bind)};
}

DbError DbError::bindNotFound(int bind) noexcept {
    return DbError{-1, eError, fmt::format("Bind at index `{}` not found", bind)};
}

DbError DbError::connectionError(std::string_view message) noexcept {
    return DbError{-1, eConnectionError, std::string{message}};
}

DbError DbError::notReady(std::string message) noexcept {
    return DbError{-1, eError, std::move(message)};
}
