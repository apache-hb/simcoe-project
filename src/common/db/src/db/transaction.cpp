#include "db/transaction.hpp"
#include "core/defer.hpp"

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

    if (DbError err = mConnection->commit()) {
        LOG_ERROR(DbLog, "Transaction::~Transaction() failed to commit: {}", err);
    }

    defer { mConnection->setAutoCommit(true); };
}

void Transaction::rollback() noexcept(false) {
    if (mState != ePending)
        return; // already committed or rolled back

    defer { mConnection->setAutoCommit(true); };

    mState = eRollback;
    mConnection->rollback().throwIfFailed();
}
