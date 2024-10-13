#include "core/string.hpp"
#include "stdafx.hpp"

#include "common.hpp"

using namespace sm;
using namespace sm::db;
using namespace sm::db::detail;

DbError IStatement::isNullByName(std::string_view column, bool& value) noexcept {
    int index = -1;
    if (DbError error = getColumnIndex(column, index))
        return error;

    return isNullByIndex(index, value);
}

/** Getting results */

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

DbError IStatement::getDateTimeByName(std::string_view column, DateTime& value) noexcept {
    return getValue(column, value, &IStatement::getDateTimeByIndex);
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
    return bindValue(name, std::move(value), &IStatement::bindBlobByIndex);
}

DbError IStatement::bindDateTimeByName(std::string_view name, DateTime value) noexcept {
    return bindValue(name, value, &IStatement::bindDateTimeByIndex);
}

DbError IStatement::bindNullByName(std::string_view name) noexcept {
    int index = -1;
    if (DbError error = findBindIndex(name, index))
        return error;

    return bindNullByIndex(index);
}

DbError IStatement::findColumnIndex(std::string_view name, int& index) const noexcept {
    return getColumnIndex(name, index);
}

DbError IStatement::findBindIndex(std::string_view name, int& index) const noexcept {
    return getBindIndex(name, index);
}

static bool isWhitespace(std::string_view str) noexcept {
    for (char c : str) {
        if (!std::isspace(c))
            return false;
    }

    return true;
}

std::vector<std::string_view> detail::splitComment(std::string_view comment) {
    std::vector<std::string_view> result;

    for (auto lines : sm::splitAll(comment, '\n')) {
        if (isWhitespace(lines))
            continue;

        trimWhitespaceFromView(lines);
        result.push_back(lines);
    }

    return result;
}
