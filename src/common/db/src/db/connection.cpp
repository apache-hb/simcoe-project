#include "stdafx.hpp"

#include "drivers/common.hpp"

#include "db/db.hpp"
#include "db/error.hpp"
#include "db/connection.hpp"

using namespace sm::db;

void detail::destroyConnection(detail::IConnection *impl) noexcept {
    delete impl;
}

bool Connection::tableExists(std::string_view name) noexcept(false) {
    std::string sql = mImpl->setupTableExists();

    PreparedStatement stmt = prepareQuery(sql);
    stmt.bind("name").to(name);
    ResultSet results = db::throwIfFailed(stmt.start());

    return results.at<int>(0) > 0;
}

bool Connection::tableExists(const dao::TableInfo& info) noexcept(false) {
    return tableExists(info.name);
}

bool Connection::userExists(std::string_view name, bool fallback) noexcept(false) {
    if (!mImpl->hasUsers())
        return fallback;

    std::string sql = mImpl->setupUserExists();

    PreparedStatement stmt = prepareQuery(sql);
    stmt.bind("name").to(name);
    ResultSet results = db::throwIfFailed(stmt.start());

    return results.at<int>(0) > 0;
}

bool Connection::hasUsers() const noexcept {
    return mImpl->hasUsers();
}

Version Connection::clientVersion() const {
    return mImpl->clientVersion();
}

Version Connection::serverVersion() const {
    return mImpl->serverVersion();
}

DbResult<PreparedStatement> Connection::tryPrepareStatement(std::string_view sql, StatementType type) noexcept try {
    return prepareStatement(sql, type);
} catch (const DbException& e) {
    return std::unexpected{e.error()};
}

PreparedStatement Connection::prepareStatement(std::string_view sql, StatementType type) throws(DbException) {
    detail::IStatement *statement = mImpl->prepare(sql);
    return PreparedStatement{statement, this, type};
}

DbResult<PreparedStatement> Connection::tryPrepareQuery(std::string_view sql) noexcept {
    return tryPrepareStatement(sql, StatementType::eQuery);
}

DbResult<PreparedStatement> Connection::tryPrepareUpdate(std::string_view sql) noexcept {
    return tryPrepareStatement(sql, StatementType::eModify);
}

DbResult<PreparedStatement> Connection::tryPrepareDefine(std::string_view sql) noexcept {
    return tryPrepareStatement(sql, StatementType::eDefine);
}

DbResult<PreparedStatement> Connection::tryPrepareControl(std::string_view sql) noexcept {
    return tryPrepareStatement(sql, StatementType::eControl);
}

PreparedStatement Connection::prepareInsertImpl(const dao::TableInfo& table) noexcept(false) {
    std::string sql = mImpl->setupInsert(table);
    return prepareUpdate(sql);
}

PreparedStatement Connection::prepareInsertOrUpdateImpl(const dao::TableInfo& table) noexcept(false) {
    std::string sql = mImpl->setupInsertOrUpdate(table);
    return prepareUpdate(sql);
}

PreparedStatement Connection::prepareInsertReturningPrimaryKeyImpl(const dao::TableInfo& table) noexcept(false) {
    CTASSERTF(table.hasAutoIncrementPrimaryKey(), "Table `%s` has no auto-increment primary key", table.name.data());
    std::string sql = mImpl->setupInsertReturningPrimaryKey(table);
    return prepareUpdate(sql);
}

PreparedStatement Connection::prepareTruncateImpl(const dao::TableInfo& table) noexcept(false) {
    std::string sql = mImpl->setupTruncate(table);
    return prepareUpdate(sql);
}

PreparedStatement Connection::prepareUpdateAllImpl(const dao::TableInfo& table) noexcept(false) {
    std::string sql = mImpl->setupUpdate(table);
    return prepareUpdate(sql);
}

PreparedStatement Connection::prepareSelectAllImpl(const dao::TableInfo& table) noexcept(false) {
    std::string sql = mImpl->setupSelect(table);
    return prepareQuery(sql);
}

PreparedStatement Connection::prepareSelectByPrimaryKeyImpl(const dao::TableInfo& table) noexcept(false) {
    std::string sql = mImpl->setupSelectByPrimaryKey(table);
    return prepareQuery(sql);
}

PreparedStatement Connection::prepareDropTableImpl(const dao::TableInfo& table) noexcept(false) {
    std::string sql = fmt::format("DROP TABLE {}", table.name);
    return prepareUpdate(sql);
}

bool Connection::createTable(const dao::TableInfo& table) noexcept(false) {
    if (tableExists(table.name))
        return false;

    updateSql(mImpl->setupCreateTable(table));

    if (mImpl->hasCommentOn()) {
        if (!table.comment.empty())
            updateSql(mImpl->setupCommentOnTable(table.name, table.comment));

        for (const auto& column : table.columns) {
            if (column.comment.empty())
                continue;

            updateSql(mImpl->setupCommentOnColumn(table.name, column.name, column.comment));
        }
    }

    if (table.isSingleton()) {
        updateSql(mImpl->setupSingletonTrigger(table));
    }

    return true;
}

void Connection::replaceTable(const dao::TableInfo& info) noexcept(false) {
    dropTableIfExists(info);
    createTable(info);
}

void Connection::dropTable(const dao::TableInfo& info) noexcept(false) {
    auto stmt = prepareDropTableImpl(info);
    stmt.execute().throwIfFailed();
}

void Connection::dropTableIfExists(const dao::TableInfo& info) noexcept(false) {
    if (tableExists(info))
        dropTable(info);
}

DbResult<ResultSet> Connection::trySelectSql(std::string_view sql) noexcept {
    PreparedStatement stmt = TRY_RESULT(tryPrepareQuery(sql));

    return stmt.start();
}

DbError Connection::tryUpdateSql(std::string_view sql) noexcept {
    PreparedStatement stmt = TRY_UNWRAP(tryPrepareUpdate(sql));

    return stmt.execute();
}

DbError Connection::begin() noexcept {
    return mImpl->begin();
}

DbError Connection::commit() noexcept {
    return mImpl->commit();
}

DbError Connection::rollback() noexcept {
    return mImpl->rollback();
}
