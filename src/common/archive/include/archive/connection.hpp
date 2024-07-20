#pragma once

#include "core/core.hpp"
#include "core/fs.hpp"
#include "core/macros.hpp"

#include <expected>

typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;

namespace sm::db {
    class ResultSet;
    class QueryIterator;
    class Query;
    class Stmt;
    class Connection;

    enum class DbError : int {
        eOk = 0
    };

    class Transaction {
        friend Connection;

        sqlite3_stmt *mStmt = nullptr;
    public:
        Transaction(sqlite3_stmt *stmt) noexcept
            : mStmt(stmt)
        { }

        ~Transaction() noexcept;

        SM_NOCOPY(Transaction);

        Transaction(Transaction&& other) noexcept
            : mStmt(other.mStmt)
        {
            other.mStmt = nullptr;
        }

        void rollback() noexcept;

        int step() noexcept;

        sqlite3_stmt *getStmt() noexcept { return mStmt; }

        std::string_view getString(int index) const noexcept;
    };

    class ResultSet {
        friend Connection;
        friend QueryIterator;

        Transaction& mTransaction;

        ResultSet(Transaction& transaction) noexcept
            : mTransaction(transaction)
        { }

    public:
        std::string_view getString(int index) const noexcept;
    };

    class QueryIterator {
        friend Query;
        Transaction *mTransaction;

        QueryIterator(Transaction *transaction) noexcept
            : mTransaction(transaction)
        { }
    public:

        QueryIterator& operator++() noexcept;
        bool operator==(const QueryIterator& other) const noexcept;
        bool operator!=(const QueryIterator& other) const noexcept;
        ResultSet operator*() const noexcept;
    };

    class Query {
        friend Connection;

        Transaction mTransaction;

        Query(sqlite3_stmt *iter) noexcept
            : mTransaction(iter)
        { }

    public:
        QueryIterator begin() noexcept { return QueryIterator{&mTransaction}; }
        QueryIterator end() noexcept { return QueryIterator{nullptr}; }
    };

    class BindPoint {
        friend Stmt;

        sqlite3_stmt *mStmt = nullptr;
        int mIndex = 0;

        BindPoint(sqlite3_stmt *stmt, int index) noexcept
            : mStmt(stmt)
            , mIndex(index)
        { }

    public:
        void to(int64 value) noexcept;
        void to(std::string_view value) noexcept;
        void to(double value) noexcept;
        void to(std::nullptr_t) noexcept;

        BindPoint& operator=(int64 value) noexcept { to(value); return *this; }
        BindPoint& operator=(std::string_view value) noexcept { to(value); return *this; }
        BindPoint& operator=(double value) noexcept { to(value); return *this; }
        BindPoint& operator=(std::nullptr_t) noexcept { to(nullptr); return *this; }
    };

    class Stmt {
        friend Connection;

        sqlite3_stmt *mStmt = nullptr;

        Stmt(sqlite3_stmt *stmt) noexcept
            : mStmt(stmt)
        { }

    public:
        BindPoint set(int index) noexcept { return BindPoint{mStmt, index}; }
        void bind(int index, int64 value) noexcept { set(index) = value; }
        void bind(int index, std::string_view value) noexcept { set(index) = value; }
        void bind(int index, double value) noexcept { set(index) = value; }
        void bind(int index, std::nullptr_t) noexcept { set(index) = nullptr; }

        BindPoint set(std::string_view name) noexcept;
        void bind(std::string_view name, int64 value) noexcept { set(name) = value; }
        void bind(std::string_view name, std::string_view value) noexcept { set(name) = value; }
        void bind(std::string_view name, double value) noexcept { set(name) = value; }
        void bind(std::string_view name, std::nullptr_t) noexcept { set(name) = nullptr; }
    };

    class Connection {
        sqlite3 *mConnection = nullptr;

        Connection(sqlite3 *connection) noexcept
            : mConnection(connection)
        { }

    public:
        static std::expected<Connection, DbError> open(const fs::path& path) noexcept;

        void close() noexcept;

        std::expected<Stmt, DbError> prepare(std::string_view sql) noexcept;

        std::expected<Transaction, DbError> execute(std::string_view sql) noexcept;

        std::expected<Query, DbError> select(std::string_view sql) noexcept;
    };

    std::string_view toString(DbError error) noexcept;
}
