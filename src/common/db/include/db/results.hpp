#pragma once

#include <simcoe_db_config.h>

#include "db/core.hpp"
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
        bool mIsDone;

        ResultSet(detail::StmtHandle impl, bool isDone = false) noexcept
            : mImpl(std::move(impl))
            , mIsDone(isDone)
        { }

        DbError getRowData(const dao::TableInfo& info, void *dst) noexcept;

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
        DbResult<Blob> getBlob(int index) noexcept;

        DbResult<double> getDouble(std::string_view column) noexcept;
        DbResult<int64> getInt(std::string_view column) noexcept;
        DbResult<bool> getBool(std::string_view column) noexcept;
        DbResult<std::string_view> getString(std::string_view column) noexcept;
        DbResult<Blob> getBlob(std::string_view column) noexcept;

        template<dao::DaoInterface T>
        DbResult<T> getRow() noexcept {
            T value;
            if (DbError error = getRowData(T::getTableInfo(), static_cast<void*>(&value)))
                return std::unexpected{error};

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

        template<>
        DbResult<std::string> get(std::string_view column) noexcept {
            return getString(column)
                .transform([](auto it) {
                    return std::string(it);
                });
        }

        template<>
        DbResult<std::string> get(int index) noexcept {
            return getString(index)
                .transform([](auto it) {
                    return std::string(it);
                });
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

#undef RESULT_SET_GET_IMPL

}
