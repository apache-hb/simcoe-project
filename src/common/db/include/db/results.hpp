#pragma once

#include <simcoe_db_config.h>

#include "db/core.hpp"
#include "db/error.hpp"

#include "core/core.hpp"

namespace sm::db {
/// @brief Represents a result set of a query.
    ///
    /// @note Not internally synchronized.
    /// @note Resetting the statement that produced the result set will invalidate the result set.
    /// @note Columns are 0-indexed.
    class ResultSet {
        friend PreparedStatement;

        detail::StmtHandle mImpl;

        ResultSet(detail::StmtHandle impl) noexcept
            : mImpl(std::move(impl))
        { }

    public:
        DbError next() noexcept;
        DbError execute() noexcept;

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
