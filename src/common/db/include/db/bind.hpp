#pragma once

#include "db/db.hpp"
#include "db/error.hpp"

#include "dao/dao.hpp"

#include "core/throws.hpp"

namespace sm::db {
    class BindPoint {
        friend PreparedStatement;

        detail::IStatement *mImpl = nullptr;
        std::string_view mName;

        BindPoint(detail::IStatement *impl, std::string_view name) noexcept
            : mImpl(impl)
            , mName(name)
        { }

    public:
        void to(int64_t value) throws(DbException) { toInt(value); }
        void to(uint64_t value) throws(DbException) { toUInt(value); }
        void to(bool value) throws(DbException) { toBool(value); }
        void to(std::string_view value) throws(DbException) { toString(value); }
        void to(double value) throws(DbException) { toDouble(value); }
        void to(Blob value) throws(DbException) { toBlob(std::move(value)); }
        void to(DateTime value) throws(DbException) { toDateTime(value); }
        void to(std::nullptr_t) throws(DbException) { toNull(); }

        void toInt(int64_t value) throws(DbException) { return tryBindInt(value).throwIfFailed(); }
        void toUInt(uint64_t value) throws(DbException) { return tryBindUInt(value).throwIfFailed(); }
        void toBool(bool value) throws(DbException) { return tryBindBool(value).throwIfFailed(); }
        void toString(std::string_view value) throws(DbException) { return tryBindString(value).throwIfFailed(); }
        void toDouble(double value) throws(DbException) { return tryBindDouble(value).throwIfFailed(); }
        void toBlob(Blob value) throws(DbException) { return tryBindBlob(std::move(value)).throwIfFailed(); }
        void toDateTime(DateTime value) throws(DbException) { return tryBindDateTime(value).throwIfFailed(); }
        void toNull() throws(DbException) { return tryBindNull().throwIfFailed(); }

        DbError tryBindInt(int64_t value) noexcept;
        DbError tryBindUInt(uint64_t value) noexcept;
        DbError tryBindBool(bool value) noexcept;
        DbError tryBindString(std::string_view value) noexcept;
        DbError tryBindDouble(double value) noexcept;
        DbError tryBindBlob(Blob value) noexcept;
        DbError tryBindDateTime(DateTime value) noexcept;
        DbError tryBindNull() noexcept;


        template<std::signed_integral T>
        void bind(T value) throws(DbException) { toInt(value); }

        template<std::unsigned_integral T>
        void bind(T value) throws(DbException) { toUInt(value); }

        template<std::floating_point T>
        void bind(T value) throws(DbException) { toDouble(value); }

        void bind(bool value) throws(DbException) { toBool(value); }
        void bind(std::string_view value) throws(DbException) { toString(value); }
        void bind(Blob value) throws(DbException) { toBlob(std::move(value)); }
        void bind(DateTime value) throws(DbException) { toDateTime(value); }
        void bind(std::nullptr_t) throws(DbException) { toNull(); }

        template<std::signed_integral T>
        DbError tryBind(T value) noexcept { return tryBindInt(value); }

        template<std::unsigned_integral T>
        DbError tryBind(T value) noexcept { return tryBindUInt(value); }

        template<std::floating_point T>
        DbError tryBind(T value) noexcept { return tryBindDouble(value); }

        DbError tryBind(bool value) noexcept { return tryBindBool(value); }
        DbError tryBind(std::string_view value) noexcept { return tryBindString(value); }
        DbError tryBind(Blob value) noexcept { return tryBindBlob(std::move(value)); }
        DbError tryBind(DateTime value) noexcept { return tryBindDateTime(value); }
        DbError tryBind(std::nullptr_t) noexcept { return tryBindNull(); }

        void operator=(int64_t value) throws(DbException) { to(value); }
        void operator=(bool value) throws(DbException) { to(value); }
        void operator=(std::string_view value) throws(DbException) { to(value); }
        void operator=(double value) throws(DbException) { to(value); }
        void operator=(Blob value) throws(DbException) { to(std::move(value)); }
        void operator=(DateTime value) throws(DbException) { to(value); }
        void operator=(std::nullptr_t) throws(DbException) { to(nullptr); }
    };
}
