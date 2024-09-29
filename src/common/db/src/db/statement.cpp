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

DbResult<ResultSet> PreparedStatement::update() noexcept {
    if (DbError error = mImpl->update(mConnection->autoCommit()))
        return std::unexpected(error);

    return ResultSet{mImpl, mConnection};
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

static DbError bindIndex(PreparedStatement& stmt, const dao::TableInfo& info, size_t index, bool returning, const void *data) noexcept {
    const auto& column = info.columns[index];
    size_t primaryKey = detail::primaryKeyIndex(info);
    if (returning && primaryKey == index)
        return DbError::ok();

    auto binding = stmt.bind(column.name);
    const void *field = static_cast<const char*>(data) + column.offset;

    switch (column.type) {
    case dao::ColumnType::eInt:
        return binding.tryBindInt(*reinterpret_cast<const int32_t*>(field));
    case dao::ColumnType::eUint:
        return binding.tryBindUInt(*reinterpret_cast<const uint32_t*>(field));
    case dao::ColumnType::eLong:
        return binding.tryBindInt(*reinterpret_cast<const int64_t*>(field));
    case dao::ColumnType::eUlong:
        return binding.tryBindUInt(*reinterpret_cast<const uint64_t*>(field));
    case dao::ColumnType::eBool:
        return binding.tryBindBool(*reinterpret_cast<const bool*>(field));
    case dao::ColumnType::eString:
        return binding.tryBindString(*reinterpret_cast<const std::string*>(field));
    case dao::ColumnType::eFloat:
        return binding.tryBindDouble(*reinterpret_cast<const float*>(field));
    case dao::ColumnType::eDouble:
        return binding.tryBindDouble(*reinterpret_cast<const double*>(field));
    case dao::ColumnType::eBlob:
        return binding.tryBindBlob(*reinterpret_cast<const Blob*>(field));
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
