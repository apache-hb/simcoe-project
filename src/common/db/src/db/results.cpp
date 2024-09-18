#include "stdafx.hpp"

#include "drivers/common.hpp"

#include "db/results.hpp"

using namespace sm;
using namespace sm::db;

///
/// result set
///

DbError ResultSet::next() noexcept {
    DbError result = mImpl->next();
    if (result.isDone())
        mIsDone = true;

    return result;
}

DbError ResultSet::execute() noexcept {
    return mImpl->execute();
}

bool ResultSet::isDone() const noexcept {
    return mIsDone;
}

int ResultSet::getColumnCount() const noexcept {
    return mImpl->getColumnCount();
}

DbResult<ColumnInfo> ResultSet::getColumnInfo(int index) const noexcept {
    ColumnInfo column;
    if (DbError error = mImpl->getColumnInfo(index, column))
        return std::unexpected(error);

    return column;
}

DbResult<double> ResultSet::getDouble(int index) noexcept {
    double value = 0.0;
    if (DbError error = mImpl->getDoubleByIndex(index, value))
        return error;

    return value;
}

DbResult<sm::int64> ResultSet::getInt(int index) noexcept {
    int64 value = 0;
    if (DbError error = mImpl->getIntByIndex(index, value))
        return error;

    return value;
}

DbResult<bool> ResultSet::getBool(int index) noexcept {
    bool value = false;
    if (DbError error = mImpl->getBooleanByIndex(index, value))
        return error;

    return value;
}

DbResult<std::string_view> ResultSet::getString(int index) noexcept {
    std::string_view value;
    if (DbError error = mImpl->getStringByIndex(index, value))
        return std::unexpected(error);

    return value;
}

DbResult<Blob> ResultSet::getBlob(int index) noexcept {
    Blob value;
    if (DbError error = mImpl->getBlobByIndex(index, value))
        return std::unexpected(error);

    return value;
}

DbResult<double> ResultSet::getDouble(std::string_view column) noexcept {
    double value = 0.0;
    if (DbError error = mImpl->getDoubleByName(column, value))
        return error;

    return value;
}

DbResult<sm::int64> ResultSet::getInt(std::string_view column) noexcept {
    int64 value = 0;
    if (DbError error = mImpl->getIntByName(column, value))
        return error;

    return value;
}

DbResult<bool> ResultSet::getBool(std::string_view column) noexcept {
    bool value = false;
    if (DbError error = mImpl->getBooleanByName(column, value))
        return error;

    return value;
}

DbResult<std::string_view> ResultSet::getString(std::string_view column) noexcept {
    std::string_view value;
    if (DbError error = mImpl->getStringByName(column, value))
        return std::unexpected(error);

    return value;
}

DbResult<Blob> ResultSet::getBlob(std::string_view column) noexcept {
    Blob value;
    if (DbError error = mImpl->getBlobByName(column, value))
        return std::unexpected(error);

    return value;
}

static DbError getColumnData(ResultSet& results, const dao::ColumnInfo& info, void *dst) noexcept {
    using enum dao::ColumnType;
    switch (info.type) {
    case eInt:
        *reinterpret_cast<int32_t*>(dst) = TRY_UNWRAP(results.get<int32_t>(info.name));
        break;
    case eUint:
        *reinterpret_cast<uint32_t*>(dst) = TRY_UNWRAP(results.get<uint32_t>(info.name));
        break;
    case eLong:
        *reinterpret_cast<int64_t*>(dst) = TRY_UNWRAP(results.get<int64_t>(info.name));
        break;
    case eUlong:
        *reinterpret_cast<uint64_t*>(dst) = TRY_UNWRAP(results.get<uint64_t>(info.name));
        break;
    case eBool:
        *reinterpret_cast<bool*>(dst) = TRY_UNWRAP(results.get<bool>(info.name));
        break;
    case eString:
        *reinterpret_cast<std::string*>(dst) = TRY_UNWRAP(results.get<std::string>(info.name));
        break;
    case eFloat:
        *reinterpret_cast<float*>(dst) = TRY_UNWRAP(results.get<double>(info.name));
        break;
    case eDouble:
        *reinterpret_cast<double*>(dst) = TRY_UNWRAP(results.get<float>(info.name));
        break;
    default:
        return DbError::todo();
    }

    return DbError::ok();
}

DbError ResultSet::getRowData(const dao::TableInfo& info, void *dst) noexcept {
    for (size_t i = 0; i < info.columns.size(); ++i) {
        auto& column = info.columns[i];
        void *col = static_cast<char*>(dst) + column.offset;
        if (DbError error = getColumnData(*this, column, col))
            return error;
    }

    return DbError::ok();
}
