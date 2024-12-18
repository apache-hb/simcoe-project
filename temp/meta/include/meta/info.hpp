#pragma once

#include <span>
#include <string_view>
#include <stdint.h>

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
        constexpr TypeInfo(TypeInfoData info) noexcept
            : mInfo(std::move(info))
        { }

    public:
        virtual ~TypeInfo() = default;

        virtual TypeClass getTypeClass() const noexcept = 0;
        virtual size_t getSizeOf() const noexcept = 0;
        virtual size_t getAlignOf() const noexcept = 0;

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
        eDiff,
        ePtr,
        eSize,

        eCount
    };

    enum class IntegerSign {
        eSigned,
        eUnsigned,

        eCount
    };

    static constexpr size_t getTableIndex(IntegerWidth width, IntegerSign sign) noexcept {
        return (size_t)width * (size_t)IntegerSign::eCount + (size_t)sign;
    }

    class IntegerType final : public TypeInfoOf<TypeClass::eInteger> {
        using Super = TypeInfoOf<TypeClass::eInteger>;

        const IntegerWidth mWidth;
        const IntegerSign mSign;

        static constexpr std::string_view kNames[(int)IntegerWidth::eCount * (int)IntegerSign::eCount] = {
            [getTableIndex(IntegerWidth::eChar, IntegerSign::eSigned)] = "char",
            [getTableIndex(IntegerWidth::eChar, IntegerSign::eUnsigned)] = "unsigned char",
            [getTableIndex(IntegerWidth::eShort, IntegerSign::eSigned)] = "short",
            [getTableIndex(IntegerWidth::eShort, IntegerSign::eUnsigned)] = "unsigned short",
            [getTableIndex(IntegerWidth::eInt, IntegerSign::eSigned)] = "int",
            [getTableIndex(IntegerWidth::eInt, IntegerSign::eUnsigned)] = "unsigned int",
            [getTableIndex(IntegerWidth::eLong, IntegerSign::eSigned)] = "long",
            [getTableIndex(IntegerWidth::eLong, IntegerSign::eUnsigned)] = "unsigned long",
            [getTableIndex(IntegerWidth::eDiff, IntegerSign::eSigned)] = "ptrdiff_t",
            [getTableIndex(IntegerWidth::eDiff, IntegerSign::eUnsigned)] = "size_t",
            [getTableIndex(IntegerWidth::ePtr, IntegerSign::eSigned)] = "intptr_t",
            [getTableIndex(IntegerWidth::ePtr, IntegerSign::eUnsigned)] = "uintptr_t",
            [getTableIndex(IntegerWidth::eSize, IntegerSign::eSigned)] = "ssize_t",
            [getTableIndex(IntegerWidth::eSize, IntegerSign::eUnsigned)] = "size_t"
        };

    public:
        constexpr IntegerType(TypeInfoData info, IntegerWidth width, IntegerSign sign) noexcept
            : Super(std::move(info))
            , mWidth(width)
            , mSign(sign)
        { }

        IntegerWidth getWidth() const noexcept { return mWidth; }
        IntegerSign getSign() const noexcept { return mSign; }

        size_t getSizeOf() const noexcept override;
        size_t getAlignOf() const noexcept override;

        static constexpr IntegerType get(IntegerWidth width, IntegerSign sign) noexcept {
            TypeInfoData info { "std", kNames[getTableIndex(width, sign)] };
            return IntegerType(info, width, sign);
        }
    };

    enum class RealWidth {
        eFloat,
        eDouble,

        eCount
    };

    class RealType : public TypeInfoOf<TypeClass::eReal> {
        const RealWidth mWidth;

        static constexpr std::string_view kNames[(int)RealWidth::eCount] = {
            [(int)RealWidth::eFloat] = "float",
            [(int)RealWidth::eDouble] = "double"
        };

    public:
        constexpr RealType(TypeInfoData info, RealWidth width) noexcept
            : TypeInfoOf(std::move(info))
            , mWidth(width)
        { }

        RealWidth getWidth() const noexcept { return mWidth; }

        size_t getSizeOf() const noexcept override;
        size_t getAlignOf() const noexcept override;

        static constexpr RealType get(RealWidth width) noexcept {
            TypeInfoData info { "std", kNames[(int)width] };
            return RealType(info, width);
        }
    };

    class StringType : public TypeInfoOf<TypeClass::eString> {

    };

    struct EnumCase {
        const std::string_view mName;
        const int64_t mValue;
    };

    class EnumType final : public TypeInfoOf<TypeClass::eEnum> {
        std::span<const EnumCase> mCases;
        IntegerType mUnderlying;

    public:
        std::span<const EnumCase> getCases() const noexcept {
            return mCases;
        }

        size_t getSizeOf() const noexcept override { return mUnderlying.getSizeOf(); }
        size_t getAlignOf() const noexcept override { return mUnderlying.getAlignOf(); }
    };

    class Field {
        const std::string_view mName;
        const size_t mOffsetOf;

    public:
        Field(std::string_view name, size_t offsetOf) noexcept
            : mName(name)
            , mOffsetOf(offsetOf)
        { }

        std::string_view getName() const noexcept { return mName; }
        size_t getOffsetOf() const noexcept { return mOffsetOf; }
    };

    class ClassType final : public TypeInfoOf<TypeClass::eClass> {
        std::span<const Field> mFields;
        const size_t mSizeOf;
        const size_t mAlignOf;

    public:
        constexpr ClassType(TypeInfoData info, std::span<const Field> fields, size_t sizeOf, size_t alignOf) noexcept
            : TypeInfoOf(std::move(info))
            , mFields(fields)
            , mSizeOf(sizeOf)
            , mAlignOf(alignOf)
        { }

        size_t getSizeOf() const noexcept override { return mSizeOf; }
        size_t getAlignOf() const noexcept override { return mAlignOf; }
    };
}
