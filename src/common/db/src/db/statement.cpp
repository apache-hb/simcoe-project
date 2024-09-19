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

    return ResultSet{mImpl};
}

DbResult<ResultSet> PreparedStatement::start() noexcept {
    DbError error = mImpl->start(mConnection->autoCommit(), type());
    if (!error.isSuccess())
        return std::unexpected(error);

    return ResultSet{mImpl, error.isDone()};
}

DbError PreparedStatement::execute() noexcept {
    auto result = TRY_UNWRAP(start());
    return result.execute();
}

static void bindIndex(PreparedStatement& stmt, const dao::TableInfo& info, size_t index, bool returning, const void *data) noexcept {
    const auto& column = info.columns[index];
    size_t primaryKey = detail::primaryKeyIndex(info);
    if (returning && primaryKey == index)
        return;

    auto binding = stmt.bind(column.name);
    const void *field = static_cast<const char*>(data) + column.offset;

    switch (column.type) {
    case dao::ColumnType::eInt:
        binding.toInt(*reinterpret_cast<const int32_t*>(field));
        break;
    case dao::ColumnType::eUint:
        binding.toInt(*reinterpret_cast<const uint32_t*>(field));
        break;
    case dao::ColumnType::eLong:
        binding.toInt(*reinterpret_cast<const int64_t*>(field));
        break;
    case dao::ColumnType::eUlong:
        binding.toInt(*reinterpret_cast<const uint64_t*>(field));
        break;
    case dao::ColumnType::eBool:
        binding.toBool(*reinterpret_cast<const bool*>(field));
        break;
    case dao::ColumnType::eString:
        binding.toString(*reinterpret_cast<const std::string*>(field));
        break;
    case dao::ColumnType::eFloat:
        binding.toDouble(*reinterpret_cast<const float*>(field));
        break;
    case dao::ColumnType::eDouble:
        binding.toDouble(*reinterpret_cast<const double*>(field));
        break;
    default:
        CT_NEVER("Unsupported column type");
    }
}

void db::bindRowToStatement(PreparedStatement& stmt, const dao::TableInfo& info, bool returning, const void *data) noexcept {
    for (size_t i = 0; i < info.columns.size(); i++)
        bindIndex(stmt, info, i, returning, data);
}
