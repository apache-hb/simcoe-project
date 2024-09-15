#pragma once

#include <span>
#include <string_view>

namespace sm::meta {
    class Class;
    class Method;
    class Property;

    enum class TypeId : uint32_t {
        eInvalid = 0,

        eBool,
        eString,
        eArray,

        eInt8,
        eInt16,
        eInt32,
        eInt64,

        eUInt8,
        eUInt16,
        eUInt32,
        eUInt64,

        eFloat,
        eDouble,

        eClass,
        eInterface,
        eEnum,

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
        constexpr std::string_view getQualifiedName() const { return mQualifiedName; }
        constexpr uint32_t getTypeId() const { return mTypeId; }
    };

    class Method : public NamedDecl {
    public:
    };

    struct PropertyType {
        enum {
            eUnknown,

            eBool,
            eString,
            eFloat,
            eDouble,

            eVector2,
            eVector3,
            eVector4,

            eQuaternion,

            eClass, // must be an instance of a given class
            eDerivedClass, // must be an instance of a class derived from a given class
            eImplements, // must implement a given interface

            eList, // list of a given type
        } kind;

        union {
            // eVector2, eVector3, eVector4
            const PropertyType *vec;

            // eQuaternion
            const PropertyType *quat;

            // eClass, eDerivedClass, eImplements
            const Class *cls;

            // eList
            const PropertyType *list;
        };
    };

    class Property : public NamedDecl {
        PropertyType mPropertyType;

    public:
        constexpr Property(PropertyType type) : mPropertyType(type) { }

        constexpr PropertyType getType() const { return mPropertyType; }
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
        const Class *mParentClass = nullptr;
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
        uint64_t value;
        std::string_view name;
    };

    class EnumInfo {
        std::string_view mName;
        std::string_view mQualifiedName;

    public:
        constexpr EnumInfo(std::string_view name, std::string_view qual)
            : mName(name)
            , mQualifiedName(qual)
        { }

        constexpr std::string_view getName() const { return mName; }
        constexpr std::string_view getQualifiedName() const { return mQualifiedName; }
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
