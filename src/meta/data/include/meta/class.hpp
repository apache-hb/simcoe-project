#pragma once

#include <span>
#include <string_view>

namespace sm::meta {
    class Class;
    class Method;
    class Property;

    enum : uint32_t {
        eTypeIdInvalid = 0,
        eTypeIdClass = 1,
        eTypeIdInterface = 2,
        eTypeIdEnum = 3,

        eTypeIdInt8 = 4,
        eTypeIdInt16 = 5,
        eTypeIdInt32 = 6,
        eTypeIdInt64 = 7,

        eTypeIdUInt8 = 8,
        eTypeIdUInt16 = 9,
        eTypeIdUInt32 = 10,
        eTypeIdUInt64 = 11,

        eTypeIdFloat = 12,
        eTypeIdDouble = 13,

        eTypeIdBool = 14,
        eTypeIdString = 15,

        eUserTypeId = 1024,
    };

    class TypeInfo {
        std::string_view mName;
        std::string_view mQualifiedName;
        std::string_view mCategory;
        uint32_t mTypeId;

    protected:
        TypeInfo() = default;

        constexpr void setName(std::string_view name) { mName = name; }
        constexpr void setQualifiedName(std::string_view qual) { mQualifiedName = qual; }
        constexpr void setCategory(std::string_view category) { mCategory = category; }
        constexpr void setTypeId(uint32_t id) { mTypeId = id; }

    public:
        constexpr std::string_view name() const { return mName; }
        constexpr std::string_view qualified() const { return mQualifiedName; }
        constexpr std::string_view category() const { return mCategory; }
        constexpr uint32_t typeId() const { return mTypeId; }
    };

    class Method {
        std::string_view mName;
        std::string_view mCategory;
    public:
        constexpr Method(std::string_view name, std::string_view category)
            : mName(name)
            , mCategory(category)
        { }

        constexpr std::string_view name() const { return mName; }
        constexpr std::string_view category() const { return mCategory; }
    };

    class Property {
        std::string_view mName;
        std::string_view mCategory;
    public:
        constexpr Property(std::string_view name, std::string_view category)
            : mName(name)
            , mCategory(category)
        { }

        constexpr std::string_view name() const { return mName; }
        constexpr std::string_view category() const { return mCategory; }
    };

    class Interface : public TypeInfo {
        std::span<const Method> mMethods;
        std::span<const Property> mProperties;

    protected:
        Interface() = default;

        constexpr void setMethods(std::span<const Method> methods) { mMethods = methods; }
        constexpr void setProperties(std::span<const Property> properties) { mProperties = properties; }

    public:
        constexpr std::span<const Method> methods() const { return mMethods; }
        constexpr std::span<const Property> properties() const { return mProperties; }
    };

    class Class : public TypeInfo {
        const Class *mParentClass;
        std::span<const Interface* const> mInterfaces;

        std::span<const Method> mMethods;
        std::span<const Property> mProperties;

    protected:
        Class() = default;

        constexpr void setParentClass(const Class *parent) { mParentClass = parent; }
        constexpr void setInterfaces(std::span<const Interface* const> interfaces) { mInterfaces = interfaces; }
        constexpr void setMethods(std::span<const Method> methods) { mMethods = methods; }
        constexpr void setProperties(std::span<const Property> properties) { mProperties = properties; }

    public:
        constexpr const Class* parent() const { return mParentClass; }
        constexpr std::span<const Interface* const> interfaces() const { return mInterfaces; }
        constexpr std::span<const Method> methods() const { return mMethods; }
        constexpr std::span<const Property> properties() const { return mProperties; }

        constexpr bool canCastTo(const Class& other) const {
            if (this == &other) return true;
            if (mParentClass) return mParentClass->canCastTo(other);
            return false;
        }
    };

    struct EnumCase {
        int64_t value;
        std::string_view name;
    };

    class Enum : public TypeInfo {
        enum Bits : int {
            eNone = 0,

            eBitFlags = (1 << 0)
        };

        std::span<const EnumCase> mCases;
        int mBits = eNone;

    protected:
        Enum() = default;

        constexpr void setCases(std::span<const EnumCase> cases) { mCases = cases; }
        constexpr void setBitFlags() { mBits |= eBitFlags; }

    public:
        constexpr std::span<const EnumCase> cases() const { return mCases; }
        constexpr bool isBitFlags() const { return mBits & eBitFlags; }
    };

    namespace detail {
        template<typename T>
        struct GlobalClass { };

        template<typename T>
        struct Reflect { };
    }

    template<typename T>
    consteval auto reflect();
}
