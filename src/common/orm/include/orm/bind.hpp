#pragma once

#include "orm/core.hpp"

namespace sm::db {
    class IBindPoint {
        detail::IStatement *mImpl = nullptr;

    protected:
        IBindPoint(detail::IStatement *impl) noexcept
            : mImpl(impl)
        { }

        detail::IStatement &impl() noexcept { return *mImpl; }

    public:
        virtual ~IBindPoint() = default;

        virtual void to(int64 value) noexcept = 0;
        virtual void to(bool value) noexcept = 0;
        virtual void to(std::string_view value) noexcept = 0;
        virtual void to(double value) noexcept = 0;
        virtual void to(Blob value) noexcept = 0;
        virtual void to(std::nullptr_t) noexcept = 0;

        void operator=(int64 value) noexcept { to(value); }
        void operator=(bool value) noexcept { to(value); }
        void operator=(std::string_view value) noexcept { to(value); }
        void operator=(double value) noexcept { to(value); }
        void operator=(Blob value) noexcept { to(value); }
        void operator=(std::nullptr_t) noexcept { to(nullptr); }
    };

    class NamedBindPoint final : public IBindPoint {
        friend PreparedStatement;

        std::string_view mName;

        NamedBindPoint(detail::IStatement *impl, std::string_view name) noexcept
            : IBindPoint(impl)
            , mName(name)
        { }

    public:
        using IBindPoint::operator=;

        void to(int64 value) noexcept override;
        void to(bool value) noexcept override;
        void to(std::string_view value) noexcept override;
        void to(double value) noexcept override;
        void to(Blob value) noexcept override;
        void to(std::nullptr_t) noexcept override;
    };

    class PositionalBindPoint final : public IBindPoint {
        friend PreparedStatement;

        int mIndex = 0;

        PositionalBindPoint(detail::IStatement *impl, int index) noexcept
            : IBindPoint(impl)
            , mIndex(index)
        { }

    public:
        using IBindPoint::operator=;

        void to(int64 value) noexcept override;
        void to(bool value) noexcept override;
        void to(std::string_view value) noexcept override;
        void to(double value) noexcept override;
        void to(Blob value) noexcept override;
        void to(std::nullptr_t) noexcept override;
    };
}