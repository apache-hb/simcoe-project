#include "stdafx.hpp"

#include "drivers/common.hpp"
#include "drivers/utils.hpp"

#include "db/statement.hpp"
#include "db/connection.hpp"
#include "db/results.hpp"

using namespace sm;
using namespace sm::db;

///
/// statement
///

BindPoint PreparedStatement::bind(std::string_view name) noexcept {
    return BindPoint{mImpl.get(), name};
}

DbError PreparedStatement::close() noexcept {
    return mImpl->finalize();
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
DbError tryBindField(BindPoint& binding, const void *field, bool nullable) noexcept {
    using Option = std::optional<T>;
    if (nullable) {
        const Option *value = reinterpret_cast<const Option*>(field);
        if (value->has_value()) {
            return binding.tryBind(value->value());
        } else {
            return binding.tryBindNull();
        }
    }

    const T *value = reinterpret_cast<const T*>(field);
    return binding.tryBind(*value);
}

static DbError bindIndex(PreparedStatement& stmt, const dao::TableInfo& info, size_t index, bool returning, const void *data) noexcept {
    const auto& column = info.columns[index];
    size_t primaryKey = detail::primaryKeyIndex(info);
    if (returning && primaryKey == index)
        return DbError::ok();

    auto binding = stmt.bind(column.name);
    const void *field = static_cast<const char*>(data) + column.offset;
    bool nullable = column.nullable;

    switch (column.type) {
    case dao::ColumnType::eInt:
        return tryBindField<int32_t>(binding, field, nullable);
    case dao::ColumnType::eUint:
        return tryBindField<uint32_t>(binding, field, nullable);
    case dao::ColumnType::eLong:
        return tryBindField<int64_t>(binding, field, nullable);
    case dao::ColumnType::eUlong:
        return tryBindField<uint64_t>(binding, field, nullable);
    case dao::ColumnType::eBool:
        return tryBindField<bool>(binding, field, nullable);
    case dao::ColumnType::eString:
        return tryBindField<std::string>(binding, field, nullable);
    case dao::ColumnType::eFloat:
        return tryBindField<float>(binding, field, nullable);
    case dao::ColumnType::eDouble:
        return tryBindField<double>(binding, field, nullable);
    case dao::ColumnType::eBlob:
        return tryBindField<db::Blob>(binding, field, nullable);
    case dao::ColumnType::eDateTime:
        return tryBindField<db::DateTime>(binding, field, nullable);
    default:
        return DbError::todo(toString(column.type));
    }
}

DbError db::bindRowToStatement(PreparedStatement& stmt, const dao::TableInfo& info, bool returning, const void *data) noexcept {
    for (size_t i = 0; i < info.columns.size(); i++)
        if (DbError error = bindIndex(stmt, info, i, returning, data))
            return error;

    return DbError::ok();
}
