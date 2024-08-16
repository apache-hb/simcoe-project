#include "orm/transaction.hpp"

using namespace sm;
using namespace sm::db;

Transaction::Transaction(Connection *conn) noexcept(false)
    : mConnection(conn)
{
    conn->begin();
    conn->setAutoCommit(false);
}

Transaction::~Transaction() noexcept(false) {
    if (mState != ePending)
        return;

    mConnection->commit();
    mConnection->setAutoCommit(true);
}

void Transaction::rollback() noexcept(false) {
    if (mState != ePending)
        return; // already committed or rolled back

    mState = eRollback;
    mConnection->rollback();

    mConnection->setAutoCommit(true);
}
