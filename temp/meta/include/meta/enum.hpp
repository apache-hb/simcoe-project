#pragma once

#include <span>
#include <string_view>

namespace sm::reflect {
    class Method;
    class Field;
    class Class;

    class EnumField {
        std::string_view mName;
        int64_t mValue;

    public:
        EnumField(std::string_view name, int64_t value) noexcept
            : mName(name)
            , mValue(value)
        { }

        std::string_view getName() const noexcept { return mName; }
        int64_t getValue() const noexcept { return mValue; }
    };

    class Enum {
        std::string_view mName;
        std::string_view mPackageName;
        std::span<const EnumField> mFields;

    protected:
        Enum(std::string_view name, std::string_view package, std::span<const EnumField> fields) noexcept
            : mName(name)
            , mPackageName(package)
            , mFields(fields)
        { }

    public:
        std::string_view getName() const noexcept { return mName; }
        std::string_view getPackageName() const noexcept { return mPackageName; }
        std::span<const EnumField> getFields() const noexcept { return mFields; }
    };

    namespace detail {
        template<typename T>
        class EnumImpl;
    }
}
