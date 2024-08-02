#pragma once

#include "orm/core.hpp"

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
        void to(int64 value) noexcept;
        void to(bool value) noexcept;
        void to(std::string_view value) noexcept;
        void to(double value) noexcept;
        void to(Blob value) noexcept;
        void to(std::nullptr_t) noexcept;

        void operator=(int64 value) noexcept { to(value); }
        void operator=(bool value) noexcept { to(value); }
        void operator=(std::string_view value) noexcept { to(value); }
        void operator=(double value) noexcept { to(value); }
        void operator=(Blob value) noexcept { to(value); }
        void operator=(std::nullptr_t) noexcept { to(nullptr); }
    };
}
