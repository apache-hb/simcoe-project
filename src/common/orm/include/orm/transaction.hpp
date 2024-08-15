#pragma once

#include "orm/connection.hpp"

namespace sm::db {
    class Transaction {
        Connection *mConnection;
        short mState = ePending;

    public:
        Transaction(Connection *conn) noexcept(false); // throws (DbException)
        ~Transaction();

        SM_NOCOPY(Transaction);
        SM_SWAP_MOVE(Transaction);

        void rollback();

        enum : short { ePending, eCommit, eRollback };

        friend void swap(Transaction& a, Transaction& b) noexcept {
            std::swap(a.mConnection, b.mConnection);
        }
    };
}
