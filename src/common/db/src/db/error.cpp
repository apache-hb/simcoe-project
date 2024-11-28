#include "stdafx.hpp"

#include "db/error.hpp"

#include "core/format.hpp"

using namespace sm;
using namespace sm::db;

DbError::DbError(int code, int status, std::string message, bool trace)
    : Super(std::move(message), trace)
    , mCode(code)
    , mStatus(status)
{ }

DbError DbError::ok() {
    return DbError{0, eOk, "OK", false};
}

DbError DbError::todo() {
    return DbError{-1, eUnimplemented, "Not implemented"};
}

DbError DbError::todo(std::string_view subject) {
    return DbError{-1, eUnimplemented, fmt::format("Not implemented: {}", subject)};
}

DbError DbError::todoFn(std::source_location location) {
    return DbError::todo(location.function_name());
}

DbError DbError::error(int code, std::string message) {
    return DbError{code, eError, std::move(message)};
}

DbError DbError::outOfMemory() {
    return DbError{-1, eError, "Out of memory"};
}

DbError DbError::noData() {
    return DbError{-1, eNoData, "No data"};
}

DbError DbError::columnIsNull(std::string_view name) {
    return DbError{-1, eError, fmt::format("Column `{}` is null", name)};
}

DbError DbError::done(int code) {
    return DbError{code, eDone, "Done", false};
}

DbError DbError::unsupported(std::string_view subject) {
    return DbError{-1, eError, fmt::format("Unsupported {}", subject)};
}

DbError DbError::unsupported(DbType type) {
    return DbError{-1, eError, fmt::format("Environment {} is not available", type)};
}

DbError DbError::columnNotFound(std::string_view column) {
    return DbError{-1, eError, fmt::format("Column not found: `{}`", column)};
}

DbError DbError::typeMismatch(std::string_view column, DataType actual, DataType expected) {
    return DbError{-1, eError, fmt::format("Column `{}` type mismatch: expected {}, column was {}", column, expected, actual)};
}

DbError DbError::bindNotFound(std::string_view bind) {
    return DbError{-1, eError, fmt::format("Named bind `{}` not found", bind)};
}

DbError DbError::bindNotFound(int bind) {
    return DbError{-1, eError, fmt::format("Bind at index `{}` not found", bind)};
}

DbError DbError::connectionError(std::string_view message) {
    return DbError{-1, eConnectionError, std::string{message}};
}

DbError DbError::notReady(std::string message) {
    return DbError{-1, eError, std::move(message)};
}

DbError DbError::invalidHost(std::string_view reason) {
    return DbError{-1, eError, fmt::format("Invalid host. {}", reason)};
}
