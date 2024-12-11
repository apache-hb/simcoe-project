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

static bool isCorrectType(detail::IConnection *connection, DataType expected, DataType actual) noexcept {
    if (isStringDataType(actual) && !connection->hasDistinctTextTypes()) {
        return isStringDataType(expected);
    }

    DataType matching = remapExpectedType(connection, expected);

    return actual == matching;
}

// TODO: deduplicate

DbError ResultSet::checkColumnAccess(int index, DataType expected) const noexcept {
    if (!mImpl->hasDataReady())
        return DbError::noData();

    if (index < 0 || index >= getColumnCount())
        return DbError::columnNotFound(fmt::format(":{}", index));

    ColumnInfo info;
    if (DbError error = mImpl->getColumnInfo(index, info))
        return error;

    if (!isCorrectType(mConnection->impl(), expected, info.type))
        return DbError::typeMismatch(info.name, info.type, expected);

    return DbError::ok();
}

DbError ResultSet::checkColumnAccess(std::string_view column, DataType expected) const noexcept {
    if (!mImpl->hasDataReady())
        return DbError::noData();

    ColumnInfo info;
    if (DbError error = mImpl->getColumnInfo(column, info))
        return error;

    if (!isCorrectType(mConnection->impl(), expected, info.type))
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

DbResult<double> ResultSet::getDouble(int index) const noexcept {
    if (DbError error = checkColumnAccess(index, DataType::eDouble))
        return std::unexpected(error);

    double value = 0.0;
    if (DbError error = mImpl->getDoubleByIndex(index, value))
        return error;

    return value;
}

DbResult<int64_t> ResultSet::getInt(int index) const noexcept {
    if (DbError error = checkColumnAccess(index, DataType::eInteger))
        return std::unexpected(error);

    int64_t value = 0;
    if (DbError error = mImpl->getIntByIndex(index, value))
        return error;

    return value;
}

DbResult<bool> ResultSet::getBool(int index) const noexcept {
    if (DbError error = checkColumnAccess(index, DataType::eBoolean))
        return std::unexpected(error);

    bool value = false;
    if (DbError error = mImpl->getBooleanByIndex(index, value))
        return error;

    return value;
}

DbResult<std::string_view> ResultSet::getString(int index) const noexcept {
    if (DbError error = checkColumnAccess(index, DataType::eVarChar))
        return std::unexpected(error);

    std::string_view value;
    if (DbError error = mImpl->getStringByIndex(index, value))
        return std::unexpected(error);

    return value;
}

DbResult<Blob> ResultSet::getBlob(int index) const noexcept {
    if (DbError error = checkColumnAccess(index, DataType::eBlob))
        return std::unexpected(error);

    Blob value;
    if (DbError error = mImpl->getBlobByIndex(index, value))
        return std::unexpected(error);

    return value;
}

DbResult<DateTime> ResultSet::getDateTime(int index) const noexcept {
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

DbResult<double> ResultSet::getDouble(std::string_view column) const noexcept {
    if (DbError error = checkColumnAccess(column, DataType::eDouble))
        return std::unexpected(error);

    double value = 0.0;
    if (DbError error = mImpl->getDoubleByName(column, value))
        return error;

    return value;
}

DbResult<int64_t> ResultSet::getInt(std::string_view column) const noexcept {
    if (DbError error = checkColumnAccess(column, DataType::eInteger))
        return std::unexpected(error);

    int64_t value = 0;
    if (DbError error = mImpl->getIntByName(column, value))
        return error;

    return value;
}

DbResult<bool> ResultSet::getBool(std::string_view column) const noexcept {
    if (DbError error = checkColumnAccess(column, DataType::eBoolean))
        return std::unexpected(error);

    bool value = false;
    if (DbError error = mImpl->getBooleanByName(column, value))
        return error;

    return value;
}

DbResult<std::string_view> ResultSet::getString(std::string_view column) const noexcept {
    if (DbError error = checkColumnAccess(column, DataType::eVarChar))
        return std::unexpected(error);

    std::string_view value;
    if (DbError error = mImpl->getStringByName(column, value))
        return std::unexpected(error);

    return value;
}

DbResult<Blob> ResultSet::getBlob(std::string_view column) const noexcept {
    if (DbError error = checkColumnAccess(column, DataType::eBlob))
        return std::unexpected(error);

    Blob value;
    if (DbError error = mImpl->getBlobByName(column, value))
        return std::unexpected(error);

    return value;
}

DbResult<DateTime> ResultSet::getDateTime(std::string_view column) const noexcept {
    if (DbError error = checkColumnAccess(column, DataType::eDateTime))
        return std::unexpected(error);

    DateTime value;
    if (DbError error = mImpl->getDateTimeByName(column, value))
        return std::unexpected(error);

    return value;
}

DbResult<bool> ResultSet::isNull(int index) const noexcept {
    if (index < 0 || index >= getColumnCount())
        return std::unexpected{DbError::columnNotFound(fmt::format(":{}", index))};

    bool value = false;
    if (DbError error = mImpl->isNullByIndex(index, value))
        return std::unexpected(error);

    return value;
}

DbResult<bool> ResultSet::isNull(std::string_view column) const noexcept {
    if (!mImpl->hasDataReady())
        return std::unexpected{DbError::noData()};

    bool value = false;
    if (DbError error = mImpl->isNullByName(column, value))
        return std::unexpected(error);

    return value;
}

template<typename T>
void setColumnField(const ResultSet& results, void *dst, std::string_view name, bool nullable) {
    using Option = std::optional<T>;
    bool isNull = results.isNull(name).value_or(false);
    if (isNull && !nullable)
        throw DbException{DbError::columnIsNull(name)};

    if (nullable) {
        Option *value = reinterpret_cast<Option*>(dst);
        if (isNull) {
            *value = std::nullopt;
        } else {
            *value = db::throwIfFailed(results.get<T>(name));
        }
    } else {
        T *value = reinterpret_cast<T*>(dst);
        *value = db::throwIfFailed(results.get<T>(name));
    }
}

static void getColumnData(const ResultSet& results, const dao::ColumnInfo& info, void *dst) {
    std::string_view name = info.name;
    bool nullable = info.nullable;

    using enum dao::ColumnType;
    switch (info.type) {
    case eInt:
        setColumnField<int32_t>(results, dst, name, nullable);
        break;
    case eUint:
        setColumnField<uint32_t>(results, dst, name, nullable);
        break;
    case eLong:
        setColumnField<int64_t>(results, dst, name, nullable);
        break;
    case eUlong:
        setColumnField<uint64_t>(results, dst, name, nullable);
        break;
    case eBool:
        setColumnField<bool>(results, dst, name, nullable);
        break;
    case eChar: case eVarChar:
        setColumnField<std::string>(results, dst, name, nullable);
        break;
    case eFloat:
        setColumnField<float>(results, dst, name, nullable);
        break;
    case eDouble:
        setColumnField<double>(results, dst, name, nullable);
        break;
    case eBlob:
        setColumnField<Blob>(results, dst, name, nullable);
        break;
    case eDateTime:
        setColumnField<DateTime>(results, dst, name, nullable);
        break;
    default:
        throw DbException{DbError::todo(toString(info.type))};
    }
}

void ResultSet::getRowData(const dao::TableInfo& info, void *dst) const {
    for (size_t i = 0; i < info.columns.size(); ++i) {
        auto& column = info.columns[i];
        void *col = static_cast<char*>(dst) + column.offset;
        getColumnData(*this, column, col);
    }
}
