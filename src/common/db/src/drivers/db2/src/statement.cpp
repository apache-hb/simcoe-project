#include "db2/db2.hpp"

using namespace sm::db;

using Db2Statement = sm::db::db2::Db2Statement;

DbError Db2Statement::finalize() noexcept {
    return DbError::ok();
}

DbError Db2Statement::start(bool autoCommit, StatementType type) noexcept {
    return DbError::ok();
}

DbError Db2Statement::execute() noexcept {
    return DbError::ok();
}

DbError Db2Statement::next() noexcept {
    return DbError::ok();
}

Db2Statement::Db2Statement(SqlStmtHandle stmt) noexcept
    : mStmtHandle(std::move(stmt))
{ }
