#pragma once

#include "meta/meta.hpp"
#include "meta/class.hpp"

#include "math/math.hpp"

#include "world/world.hpp"

#include "object.reflect.h"

namespace sm::game {
    class Object;

    template<typename T>
    concept IsObject = requires {
        std::is_base_of_v<Object, T>;
        { T::getClass() } -> std::same_as<const meta::Class&>;
    };

    struct Archetype {
        const meta::Class& mClass;

        sm::Vector<Object*> mDefaultObjects;
    };

    class ClassSetup {
        const meta::Class& mClass;
        Object *mObject = nullptr;

    public:
        template<IsObject T>
        T *addDefaultObject(std::string_view name);

        void init(Object& object);

        ClassSetup(const meta::Class& cls);
    };

    /// @brief base object class
    /// opts in for reflection, serialization, and archetypes
    REFLECT()
    class Object {
        REFLECT_BODY(Object)

        uint32 mObjectId;

    public:
        virtual ~Object() = default;

        Object(ClassSetup& config);

        virtual void update(float dt) { }
        virtual void create() { }
        virtual void destroy() { }

        bool isInstanceOf(const meta::Class& cls) const;

        uint32 getObjectId() const { return mObjectId; }
    };

    template<IsObject T>
    T *cast(Object& object) {
        if (object.isInstanceOf(T::getClass())) {
            return static_cast<T*>(&object);
        } else {
            return nullptr;
        }
    }
}
