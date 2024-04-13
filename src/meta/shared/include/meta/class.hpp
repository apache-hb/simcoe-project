#pragma once

#include <span>
#include <string_view>

namespace sm::meta {
    class Class;
    class Method;
    class Property;

    enum class TypeId : uint32_t {
        eInvalid = 0,
        eClass = 1,
        eInterface = 2,
        eEnum = 3,

        eInt8 = 4,
        eInt16 = 5,
        eInt32 = 6,
        eInt64 = 7,

        eUInt8 = 8,
        eUInt16 = 9,
        eUInt32 = 10,
        eUInt64 = 11,

        eFloat = 12,
        eDouble = 13,

        eBool = 14,
        eString = 15,

        eUserTypeId = 1024,
    };

    enum class EnumFlags : uint32_t {
        eNone = 0,
        eBitFlags = (1 << 0)
    };

    class NamedDecl {
        std::string_view mName;
        std::string_view mCategory;

    protected:
        NamedDecl() = default;

        void setName(std::string_view name) { mName = name; }
        void setCategory(std::string_view category) { mCategory = category; }

    public:
        constexpr std::string_view getName() const { return mName; }
        constexpr std::string_view getCategory() const { return mCategory; }
    };

    class TypeInfo : public NamedDecl {
        std::string_view mQualifiedName;
        uint32_t mTypeId;

    protected:
        TypeInfo() = default;

        constexpr void setQualifiedName(std::string_view qual) { mQualifiedName = qual; }
        constexpr void setTypeId(uint32_t id) { mTypeId = id; }

    public:
        constexpr std::string_view qualified() const { return mQualifiedName; }
        constexpr uint32_t typeId() const { return mTypeId; }
    };

    class Method : public NamedDecl {
    public:
    };

    class Property : public NamedDecl {
        const TypeInfo& mType;

    public:
        constexpr Property(const TypeInfo& type) : mType(type) { }

        constexpr const TypeInfo& getType() const { return mType; }
    };

    class Interface : public TypeInfo {
        std::span<const Method> mMethods;
        std::span<const Property> mProperties;

    protected:
        Interface() = default;

        constexpr void setMethods(std::span<const Method> methods) { mMethods = methods; }
        constexpr void setProperties(std::span<const Property> properties) { mProperties = properties; }

    public:
        constexpr std::span<const Method> getMethods() const { return mMethods; }
        constexpr std::span<const Property> getProperties() const { return mProperties; }
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
        constexpr const Class* getParent() const { return mParentClass; }
        constexpr std::span<const Interface* const> getInterfaces() const { return mInterfaces; }
        constexpr std::span<const Method> getMethods() const { return mMethods; }
        constexpr std::span<const Property> getProperties() const { return mProperties; }

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
        std::span<const EnumCase> mCases;
        EnumFlags mFlags = EnumFlags::eNone;

    protected:
        Enum() = default;

        constexpr void setCases(std::span<const EnumCase> cases) { mCases = cases; }
        constexpr void setBitFlags() { mFlags = (EnumFlags)((uint32_t)mFlags | (uint32_t)EnumFlags::eBitFlags); }

    public:
        constexpr std::span<const EnumCase> cases() const { return mCases; }
        constexpr bool isBitFlags() const { return (uint32_t)mFlags & (uint32_t)EnumFlags::eBitFlags; }
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
