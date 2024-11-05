#include "stdafx.hpp"

#include "drivers/common.hpp"
#include "drivers/utils.hpp"

#include "db/statement.hpp"
#include "db/connection.hpp"
#include "db/results.hpp"

using namespace sm;
using namespace sm::db;

void detail::destroyStatement(detail::IStatement *impl) noexcept {
    delete impl;
}

void PreparedStatement::prepareIntReturn(std::string_view name) noexcept(false) {
    mImpl->prepareIntReturnByName(name).throwIfFailed();
}

void PreparedStatement::prepareStringReturn(std::string_view name) noexcept(false) {
    mImpl->prepareStringReturnByName(name).throwIfFailed();
}

BindPoint PreparedStatement::bind(std::string_view name) noexcept {
    return BindPoint{mImpl.get(), name};
}

DbResult<ResultSet> PreparedStatement::start() noexcept {
    DbError error = mImpl->start(mConnection->autoCommit(), type());
    if (!error.isSuccess())
        return std::unexpected(error);

    return ResultSet{mImpl, mConnection, error.isDone()};
}

DbError PreparedStatement::execute() noexcept {
    auto result = TRY_UNWRAP(start());
    return mImpl->execute();
}

template<typename T>
void tryBindField(BindPoint& binding, const void *field, bool nullable) {
    using Option = std::optional<T>;
    if (nullable) {
        const Option *value = reinterpret_cast<const Option*>(field);
        if (value->has_value()) {
            binding.bind(value->value());
        } else {
            binding.bind(nullptr);
        }
    } else {
        const T *value = reinterpret_cast<const T*>(field);
        binding.bind(*value);
    }
}

static void bindIndex(PreparedStatement& stmt, const dao::TableInfo& info, size_t index, bool returning, const void *data) {
    const auto& column = info.columns[index];
    size_t primaryKey = detail::primaryKeyIndex(info);
    if (returning && primaryKey == index)
        return;

    auto binding = stmt.bind(column.name);
    const void *field = static_cast<const char*>(data) + column.offset;
    bool nullable = column.nullable;

    switch (column.type) {
    case dao::ColumnType::eInt:
        tryBindField<int32_t>(binding, field, nullable);
        break;
    case dao::ColumnType::eUint:
        tryBindField<uint32_t>(binding, field, nullable);
        break;
    case dao::ColumnType::eLong:
        tryBindField<int64_t>(binding, field, nullable);
        break;
    case dao::ColumnType::eUlong:
        tryBindField<uint64_t>(binding, field, nullable);
        break;
    case dao::ColumnType::eBool:
        tryBindField<bool>(binding, field, nullable);
        break;
    case dao::ColumnType::eChar: case dao::ColumnType::eVarChar:
        tryBindField<std::string>(binding, field, nullable);
        break;
    case dao::ColumnType::eFloat:
        tryBindField<float>(binding, field, nullable);
        break;
    case dao::ColumnType::eDouble:
        tryBindField<double>(binding, field, nullable);
        break;
    case dao::ColumnType::eBlob:
        tryBindField<db::Blob>(binding, field, nullable);
        break;
    case dao::ColumnType::eDateTime:
        tryBindField<db::DateTime>(binding, field, nullable);
        break;
    default:
        throw DbException{DbError::todo(toString(column.type))};
    }
}

void db::bindRowToStatement(PreparedStatement& stmt, const dao::TableInfo& info, bool returning, const void *data) noexcept(false) {
    for (size_t i = 0; i < info.columns.size(); i++)
        bindIndex(stmt, info, i, returning, data);

    if (returning && info.hasPrimaryKey()) {
        size_t pkIndex = detail::primaryKeyIndex(info);
        const auto& column = info.columns[pkIndex];

        switch (column.type) {
        case dao::ColumnType::eInt:
        case dao::ColumnType::eUint:
        case dao::ColumnType::eLong:
        case dao::ColumnType::eUlong:
            stmt.prepareIntReturn(column.name);
            break;

        case dao::ColumnType::eChar: case dao::ColumnType::eVarChar:
            stmt.prepareStringReturn(column.name);
            break;

        default:
            throw DbException{DbError::unsupported(fmt::format("returning primary key of type {}", toString(column.type)))};
        }
    }
}
