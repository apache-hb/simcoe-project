#include "orm/transaction.hpp"

using namespace sm;
using namespace sm::db;

Transaction::Transaction(Connection *conn) noexcept
    : mConnection(conn)
{
    if (DbError err = mConnection->begin())
        CT_NEVER("Failed to start transaction: %s", err.message().data());
}

Transaction::~Transaction() noexcept {
    if (mState != ePending)
        return;

    if (DbError err = mConnection->commit())
        CT_NEVER("Failed to rollback transaction: %s", err.message().data());
}

DbError Transaction::rollback() noexcept {
    if (mState != ePending)
        return DbError::error(1, "Transaction already closed");

    mState = eRollback;
    return mConnection->rollback();
}
