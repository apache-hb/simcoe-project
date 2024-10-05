#include "stdafx.hpp"

#include "drivers/common.hpp"

#include "db/results.hpp"
#include "db/connection.hpp"

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


static DataType remapExpectedType(detail::IConnection *connection, DataType expected) noexcept {
    switch (expected) {
    case DataType::eBoolean:
        return connection->boolEquivalentType();
    case DataType::eDateTime:
        return connection->dateTimeEquivalentType();
    default:
        return expected;
    }
}

// TODO: deduplicate

DbError ResultSet::checkColumnAccess(int index, DataType expected) noexcept {
    if (!mImpl->hasDataReady())
        return DbError::noData();

    if (index < 0 || index >= getColumnCount())
        return DbError::columnNotFound(fmt::format(":{}", index));

    ColumnInfo info;
    if (DbError error = mImpl->getColumnInfo(index, info))
        return error;

    DataType matching = remapExpectedType(mConnection->impl(), expected);

    if (info.type != matching)
        return DbError::typeMismatch(info.name, info.type, expected);

    return DbError::ok();
}

DbError ResultSet::checkColumnAccess(std::string_view column, DataType expected) noexcept {
    if (!mImpl->hasDataReady())
        return DbError::noData();

    ColumnInfo info;
    if (DbError error = mImpl->getColumnInfo(column, info))
        return error;

    DataType matching = remapExpectedType(mConnection->impl(), expected);

    if (info.type != matching)
        return DbError::typeMismatch(info.name, info.type, expected);

    return DbError::ok();
}

DbResult<ColumnInfo> ResultSet::getColumnInfo(int index) const noexcept {
    if (index < 0 || index >= getColumnCount())
        return std::unexpected{DbError::columnNotFound(fmt::format(":{}", index))};

    ColumnInfo column;
    if (DbError error = mImpl->getColumnInfo(index, column))
        return std::unexpected(error);

    return column;
}

DbResult<double> ResultSet::getDouble(int index) noexcept {
    if (DbError error = checkColumnAccess(index, DataType::eDouble))
        return std::unexpected(error);

    double value = 0.0;
    if (DbError error = mImpl->getDoubleByIndex(index, value))
        return error;

    return value;
}

DbResult<sm::int64> ResultSet::getInt(int index) noexcept {
    if (DbError error = checkColumnAccess(index, DataType::eInteger))
        return std::unexpected(error);

    int64 value = 0;
    if (DbError error = mImpl->getIntByIndex(index, value))
        return error;

    return value;
}

DbResult<bool> ResultSet::getBool(int index) noexcept {
    if (DbError error = checkColumnAccess(index, DataType::eBoolean))
        return std::unexpected(error);

    bool value = false;
    if (DbError error = mImpl->getBooleanByIndex(index, value))
        return error;

    return value;
}

DbResult<std::string_view> ResultSet::getString(int index) noexcept {
    if (DbError error = checkColumnAccess(index, DataType::eString))
        return std::unexpected(error);

    std::string_view value;
    if (DbError error = mImpl->getStringByIndex(index, value))
        return std::unexpected(error);

    return value;
}

DbResult<Blob> ResultSet::getBlob(int index) noexcept {
    if (DbError error = checkColumnAccess(index, DataType::eBlob))
        return std::unexpected(error);

    Blob value;
    if (DbError error = mImpl->getBlobByIndex(index, value))
        return std::unexpected(error);

    return value;
}

DbResult<DateTime> ResultSet::getDateTime(int index) noexcept {
    if (DbError error = checkColumnAccess(index, DataType::eDateTime))
        return std::unexpected(error);

    DateTime value;
    if (DbError error = mImpl->getDateTimeByIndex(index, value))
        return std::unexpected(error);

    return value;
}

int64_t ResultSet::getIntReturn(int index) {
    return mImpl->getIntReturnByIndex(index);
}

std::string_view ResultSet::getStringReturn(int index) {
    return mImpl->getStringReturnByIndex(index);
}

DbResult<double> ResultSet::getDouble(std::string_view column) noexcept {
    if (DbError error = checkColumnAccess(column, DataType::eDouble))
        return std::unexpected(error);

    double value = 0.0;
    if (DbError error = mImpl->getDoubleByName(column, value))
        return error;

    return value;
}

DbResult<sm::int64> ResultSet::getInt(std::string_view column) noexcept {
    if (DbError error = checkColumnAccess(column, DataType::eInteger))
        return std::unexpected(error);

    int64 value = 0;
    if (DbError error = mImpl->getIntByName(column, value))
        return error;

    return value;
}

DbResult<bool> ResultSet::getBool(std::string_view column) noexcept {
    if (DbError error = checkColumnAccess(column, DataType::eBoolean))
        return std::unexpected(error);

    bool value = false;
    if (DbError error = mImpl->getBooleanByName(column, value))
        return error;

    return value;
}

DbResult<std::string_view> ResultSet::getString(std::string_view column) noexcept {
    if (DbError error = checkColumnAccess(column, DataType::eString))
        return std::unexpected(error);

    std::string_view value;
    if (DbError error = mImpl->getStringByName(column, value))
        return std::unexpected(error);

    return value;
}

DbResult<Blob> ResultSet::getBlob(std::string_view column) noexcept {
    if (DbError error = checkColumnAccess(column, DataType::eBlob))
        return std::unexpected(error);

    Blob value;
    if (DbError error = mImpl->getBlobByName(column, value))
        return std::unexpected(error);

    return value;
}

DbResult<DateTime> ResultSet::getDateTime(std::string_view column) noexcept {
    if (DbError error = checkColumnAccess(column, DataType::eDateTime))
        return std::unexpected(error);

    DateTime value;
    if (DbError error = mImpl->getDateTimeByName(column, value))
        return std::unexpected(error);

    return value;
}

DbResult<bool> ResultSet::isNull(int index) noexcept {
    if (index < 0 || index >= getColumnCount())
        return std::unexpected{DbError::columnNotFound(fmt::format(":{}", index))};

    bool value = false;
    if (DbError error = mImpl->isNullByIndex(index, value))
        return std::unexpected(error);

    return value;
}

DbResult<bool> ResultSet::isNull(std::string_view column) noexcept {
    if (!mImpl->hasDataReady())
        return std::unexpected{DbError::noData()};

    bool value = false;
    if (DbError error = mImpl->isNullByName(column, value))
        return std::unexpected(error);

    return value;
}

template<typename T>
DbError setColumnField(ResultSet& results, void *dst, std::string_view name, bool nullable) noexcept {
    using Option = std::optional<T>;
    bool isNull = results.isNull(name).value_or(false);
    if (nullable) {
        Option *value = reinterpret_cast<Option*>(dst);
        if (isNull) {
            *value = std::nullopt;
        } else {
            *value = TRY_UNWRAP(results.get<T>(name));
        }
    } else {
        T *value = reinterpret_cast<T*>(dst);
        if (isNull)
            return DbError::columnIsNull(name);

        *value = TRY_UNWRAP(results.get<T>(name));
    }

    return DbError::ok();
}

static DbError getColumnData(ResultSet& results, const dao::ColumnInfo& info, void *dst) noexcept {
    std::string_view name = info.name;
    bool nullable = info.nullable;

    using enum dao::ColumnType;
    switch (info.type) {
    case eInt:
        return setColumnField<int32_t>(results, dst, name, nullable);
    case eUint:
        return setColumnField<uint32_t>(results, dst, name, nullable);
    case eLong:
        return setColumnField<int64_t>(results, dst, name, nullable);
    case eUlong:
        return setColumnField<uint64_t>(results, dst, name, nullable);
    case eBool:
        return setColumnField<bool>(results, dst, name, nullable);
    case eString:
        return setColumnField<std::string>(results, dst, name, nullable);
    case eFloat:
        return setColumnField<float>(results, dst, name, nullable);
    case eDouble:
        return setColumnField<double>(results, dst, name, nullable);
    case eBlob:
        return setColumnField<Blob>(results, dst, name, nullable);
    case eDateTime:
        return setColumnField<DateTime>(results, dst, name, nullable);
    default:
        return DbError::todo(toString(info.type));
    }
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
