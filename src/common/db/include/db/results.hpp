#pragma once

#include <simcoe_db_config.h>

#include "db/db.hpp"
#include "db/error.hpp"

#include "core/core.hpp"

#include "dao/dao.hpp"

namespace sm::db {
    /// @brief Represents a result set of a query.
    ///
    /// @note Not internally synchronized.
    /// @note Resetting the statement that produced the result set will invalidate the result set.
    /// @note Columns are 0-indexed.
    class ResultSet {
        friend PreparedStatement;

        detail::StmtHandle mImpl;
        Connection *mConnection;
        bool mIsDone;

        ResultSet(detail::StmtHandle impl, Connection *connection, bool isDone = false) noexcept
            : mImpl(std::move(impl))
            , mConnection(connection)
            , mIsDone(isDone)
        { }

        void getRowData(const dao::TableInfo& info, void *dst);

        DbError checkColumnAccess(int index, DataType expected) noexcept;
        DbError checkColumnAccess(std::string_view column, DataType expected) noexcept;

    public:
        class EndSentinel { };

        class Iterator {
        protected:
            ResultSet *mResultSet;
            DbError mError;

        public:
            Iterator(ResultSet *resultSet) noexcept
                : mResultSet(resultSet)
                , mError(DbError::ok())
            { }

            Iterator &operator++() noexcept {
                mError = mResultSet->next();
                return *this;
            }

            bool operator!=(const EndSentinel&) const noexcept {
                return !mError.isDone();
            }

            ResultSet &operator*() noexcept {
                return *mResultSet;
            }
        };

        Iterator begin() noexcept { return Iterator(this); }
        EndSentinel end() noexcept { return EndSentinel(); }

        DbError next() noexcept;
        DbError execute() noexcept;
        bool isDone() const noexcept;

        int getColumnCount() const noexcept;
        DbResult<ColumnInfo> getColumnInfo(int index) const noexcept;

        DbResult<double> getDouble(int index) noexcept;
        DbResult<int64> getInt(int index) noexcept;
        DbResult<bool> getBool(int index) noexcept;

        DbResult<std::string_view> getString(int index) noexcept;
        DbResult<std::string_view> getVarChar(int index) noexcept;
        DbResult<std::string_view> getChar(int index) noexcept;

        DbResult<Blob> getBlob(int index) noexcept;
        DbResult<DateTime> getDateTime(int index) noexcept;

        int64_t getIntReturn(int index);
        std::string_view getStringReturn(int index);

        DbResult<double> getDouble(std::string_view column) noexcept;
        DbResult<int64> getInt(std::string_view column) noexcept;
        DbResult<bool> getBool(std::string_view column) noexcept;

        DbResult<std::string_view> getString(std::string_view column) noexcept;
        DbResult<std::string_view> getVarChar(std::string_view column) noexcept;
        DbResult<std::string_view> getChar(std::string_view column) noexcept;


        DbResult<Blob> getBlob(std::string_view column) noexcept;
        DbResult<DateTime> getDateTime(std::string_view column) noexcept;

        DbResult<bool> isNull(int index) noexcept;
        DbResult<bool> isNull(std::string_view column) noexcept;

        template<typename T>
        T getReturn(int index) = delete;

        template<std::integral T>
        T getReturn(int index) {
            return static_cast<T>(getIntReturn(index));
        }

        template<typename T> requires std::same_as<T, std::string_view>
        std::string_view getReturn(int index) {
            return getStringReturn(index);
        }

        template<dao::DaoInterface T>
        T getRow() {
            T value;
            getRowData(T::table(), static_cast<void*>(&value));
            return value;
        }

        template<typename T>
        DbResult<T> get(std::string_view column) noexcept;

        template<typename T>
        DbResult<T> get(int index) noexcept;

        template<std::integral T>
        DbResult<T> get(int column) noexcept {
            return getInt(column)
                .transform([](auto it) {
                    return static_cast<T>(it);
                });
        }

        template<std::floating_point T>
        DbResult<T> get(int column) noexcept {
            return getDouble(column)
                .transform([](auto it) {
                    return static_cast<T>(it);
                });
        }

        template<std::integral T>
        DbResult<T> get(std::string_view column) noexcept {
            return getInt(column)
                .transform([](auto it) {
                    return static_cast<T>(it);
                });
        }

        template<std::floating_point T>
        DbResult<T> get(std::string_view column) noexcept {
            return getDouble(column)
                .transform([](auto it) {
                    return static_cast<T>(it);
                });
        }

        template<typename T> requires std::same_as<T, std::string>
        DbResult<std::string> get(std::string_view column) noexcept {
            return getString(column)
                .transform([](auto it) {
                    return std::string(it);
                });
        }

        template<typename T> requires std::same_as<T, std::string>
        DbResult<std::string> get(int index) noexcept {
            return getString(index)
                .transform([](auto it) {
                    return std::string(it);
                });
        }

        template<typename T>
        T at(int index) {
            return db::throwIfFailed(get<T>(index));
        }

        template<typename T>
        T at(std::string_view column) {
            return db::throwIfFailed(get<T>(column));
        }
    };

#define RESULT_SET_GET_IMPL(type, method) \
    template<> \
    inline DbResult<type> ResultSet::get<type>(std::string_view column) noexcept { \
        return method(column); \
    } \
    template<> \
    inline DbResult<type> ResultSet::get<type>(int index) noexcept { \
        return method(index); \
    }

    RESULT_SET_GET_IMPL(bool, getBool);
    RESULT_SET_GET_IMPL(std::string_view, getString);
    RESULT_SET_GET_IMPL(Blob, getBlob);
    RESULT_SET_GET_IMPL(DateTime, getDateTime);

#undef RESULT_SET_GET_IMPL

}
