#include "orm/transaction.hpp"

using namespace sm;
using namespace sm::db;

Transaction::Transaction(Connection *conn) noexcept(false)
    : mConnection(conn)
{
    conn->begin();
    conn->setAutoCommit(false);
}

Transaction::~Transaction() noexcept {
    if (mState != ePending)
        return;

    try {
        mConnection->commit();
    } catch (const std::exception& err) {
        // TODO: use proper logging
        fmt::println(stderr, "Transaction::~Transaction() failed to commit: {}", err.what());
    } catch (...) {
        fmt::println(stderr, "Transaction::~Transaction() failed to commit *unknown*");
    }

    mConnection->setAutoCommit(true);
}

void Transaction::rollback() noexcept(false) {
    if (mState != ePending)
        return; // already committed or rolled back

    mState = eRollback;
    mConnection->rollback();

    mConnection->setAutoCommit(true);
}
