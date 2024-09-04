#include "stdafx.hpp"

#include "common.hpp"

using namespace sm;
using namespace sm::db;
using namespace sm::db::detail;

DbError IStatement::getIntByName(std::string_view column, int64& value) noexcept {
    return getValue(column, value, &IStatement::getIntByIndex);
}

DbError IStatement::getBooleanByName(std::string_view column, bool& value) noexcept {
    return getValue(column, value, &IStatement::getBooleanByIndex);
}

DbError IStatement::getStringByName(std::string_view column, std::string_view& value) noexcept {
    return getValue(column, value, &IStatement::getStringByIndex);
}

DbError IStatement::getDoubleByName(std::string_view column, double& value) noexcept {
    return getValue(column, value, &IStatement::getDoubleByIndex);
}

DbError IStatement::getBlobByName(std::string_view column, Blob& value) noexcept {
    return getValue(column, value, &IStatement::getBlobByIndex);
}

/** Binding */

DbError IStatement::bindIntByName(std::string_view name, int64 value) noexcept {
    return bindValue(name, value, &IStatement::bindIntByIndex);
}

DbError IStatement::bindBooleanByName(std::string_view name, bool value) noexcept {
    return bindValue(name, value, &IStatement::bindBooleanByIndex);
}

DbError IStatement::bindStringByName(std::string_view name, std::string_view value) noexcept {
    return bindValue(name, value, &IStatement::bindStringByIndex);
}

DbError IStatement::bindDoubleByName(std::string_view name, double value) noexcept {
    return bindValue(name, value, &IStatement::bindDoubleByIndex);
}

DbError IStatement::bindBlobByName(std::string_view name, Blob value) noexcept {
    return bindValue(name, value, &IStatement::bindBlobByIndex);
}

DbError IStatement::bindNullByName(std::string_view name) noexcept {
    int index = TRY_UNWRAP(findColumnIndex(name));
    return bindNullByIndex(index);
}

std::expected<int, DbError> IStatement::findColumnIndex(std::string_view name) const noexcept {
    int index = -1;
    if (DbError error = getColumnIndex(name, index))
        return error;

    return index;
}

std::expected<int, DbError> IStatement::findBindIndex(std::string_view name) const noexcept {
    int index = -1;
    if (DbError error = getBindIndex(name, index))
        return error;

    return index;
}