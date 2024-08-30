#pragma once

#include "orm/core.hpp"
#include "core/throws.hpp"

namespace sm::db {
    class BindPoint {
        friend PreparedStatement;

        detail::IStatement *mImpl = nullptr;
        const std::string_view mName;

        BindPoint(detail::IStatement *impl, std::string_view name) noexcept
            : mImpl(impl)
            , mName(name)
        { }

    public:
        void to(int64 value) throws(DbException);
        void to(bool value) throws(DbException);
        void to(std::string_view value) throws(DbException);
        void to(double value) throws(DbException);
        void to(Blob value) throws(DbException);
        void to(std::nullptr_t) throws(DbException);

        void operator=(int64 value) throws(DbException) { to(value); }
        void operator=(bool value) throws(DbException) { to(value); }
        void operator=(std::string_view value) throws(DbException) { to(value); }
        void operator=(double value) throws(DbException) { to(value); }
        void operator=(Blob value) throws(DbException) { to(value); }
        void operator=(std::nullptr_t) throws(DbException) { to(nullptr); }
    };
}
