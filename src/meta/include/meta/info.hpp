#pragma once

#include <span>

namespace sm::reflect {
    enum class TypeClass {
        eInteger,
        eReal,
        eBool,
        eString,
        eEnum,
        eClass
    };

    struct TypeInfoData {
        const std::string_view package;
        const std::string_view name;
    };

    class TypeInfo {
        const TypeInfoData mInfo;

    protected:
        constexpr TypeInfo(const TypeInfoData& info) noexcept
            : mInfo(info)
        { }

    public:
        virtual TypeClass getTypeClass() const noexcept = 0;

        std::string_view getPackage() const noexcept { return mInfo.package; }
        std::string_view getName() const noexcept { return mInfo.name; }
    };

    template<TypeClass C>
    class TypeInfoOf : public TypeInfo {
        TypeClass getTypeClass() const noexcept final override { return C; }
    protected:
        using TypeInfo::TypeInfo;
    };

    enum class IntegerWidth {
        eChar,
        eShort,
        eInt,
        eLong,
        eSize
    };

    enum class IntegerSign {
        eSigned,
        eUnsigned
    };

    class IntegerType final : public TypeInfoOf<TypeClass::eInteger> {
        using Super = TypeInfoOf<TypeClass::eInteger>;
        const IntegerWidth mWidth;
        const IntegerSign mSign;

    public:
        IntegerType(const TypeInfoData& info, IntegerWidth width, IntegerSign sign) noexcept
            : Super(info)
            , mWidth(width)
            , mSign(sign)
        { }

        IntegerWidth getWidth() const noexcept { return mWidth; }
        IntegerSign getSign() const noexcept { return mSign; }
    };

    class RealType : public TypeInfoOf<TypeClass::eReal> {

    };

    class StringType : public TypeInfoOf<TypeClass::eString> {

    };

    struct EnumCase {
        const std::string_view mName;
        const size_t mValue;
    };

    class EnumType final : public TypeInfoOf<TypeClass::eEnum> {
        const std::vector<EnumCase> mCases;

    public:
        std::span<const EnumCase> getCases() const noexcept {
            return mCases;
        }
    };

    class Field {
        const std::string_view mName;
    };

    class ClassType : public TypeInfoOf<TypeClass::eClass> {

    };
}
