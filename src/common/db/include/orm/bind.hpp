#pragma once

#include "orm/core.hpp"
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
        void to(int64 value) throws(DbException) { toInt(value); }
        void to(uint64 value) throws(DbException) { toUInt(value); }
        void to(bool value) throws(DbException) { toBool(value); }
        void to(std::string_view value) throws(DbException) { toString(value); }
        void to(double value) throws(DbException) { toDouble(value); }
        void to(Blob value) throws(DbException) { toBlob(value); }
        void to(std::nullptr_t) throws(DbException) { toNull(); }

        void toInt(int64 value) throws(DbException);
        void toUInt(uint64 value) throws(DbException);
        void toBool(bool value) throws(DbException);
        void toString(std::string_view value) throws(DbException);
        void toDouble(double value) throws(DbException);
        void toBlob(Blob value) throws(DbException);
        void toNull() throws(DbException);

        void operator=(int64 value) throws(DbException) { to(value); }
        void operator=(bool value) throws(DbException) { to(value); }
        void operator=(std::string_view value) throws(DbException) { to(value); }
        void operator=(double value) throws(DbException) { to(value); }
        void operator=(Blob value) throws(DbException) { to(value); }
        void operator=(std::nullptr_t) throws(DbException) { to(nullptr); }
    };
}
