#pragma once

#include "orm/connection.hpp"

namespace sm::db {
    class Transaction {
        Connection *mConnection;
        short mState = ePending;

    public:
        Transaction(Connection *conn) throws(DbException);
        ~Transaction() throws(DbException);

        SM_NOCOPY(Transaction);
        SM_SWAP_MOVE(Transaction);

        void rollback() throws(DbException);

        enum : short { ePending, eCommit, eRollback };

        friend void swap(Transaction& a, Transaction& b) noexcept {
            std::swap(a.mConnection, b.mConnection);
        }
    };
}
