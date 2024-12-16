#include "stdafx.hpp"

#include "db/transaction.hpp"

#include "base/defer.hpp"

using namespace sm;
using namespace sm::db;

Transaction::Transaction(Connection *conn) noexcept(false)
    : mConnection(conn)
{
    if (auto err = conn->begin()) {
        mState = eRollback;
        err.raise();
    }

    conn->setAutoCommit(false);
}

Transaction::~Transaction() noexcept {
    if (mState != ePending)
        return;

    defer { mConnection->setAutoCommit(true); };

    if (DbError err = mConnection->commit()) {
        LOG_ERROR(DbLog, "Transaction::~Transaction() failed to commit: {}", err);
    }
}

void Transaction::rollback() noexcept(false) {
    if (mState != ePending)
        return; // already committed or rolled back

    defer { mConnection->setAutoCommit(true); };

    mState = eRollback;
    mConnection->rollback().throwIfFailed();
}
