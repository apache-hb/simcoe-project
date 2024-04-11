#pragma once

#include "meta/meta.hpp"
#include "meta/class.hpp"

#include "math/math.hpp"

#include "world/world.hpp"

namespace sm::game {
    class Object;

    template<typename T>
    concept IsObject = requires {
        std::is_base_of_v<Object, T>;
        { T::getClass() } -> std::same_as<const meta::Class&>;
    };

    /// @brief base object class
    /// opts in for reflection, serialization, and archetypes
    REFLECT()
    class Object {
        REFLECT_BODY(Object)

        uint16 mClassId;

    public:
        virtual ~Object() = default;

        Object();

        virtual void update(float dt) { }
        virtual void create() { }
        virtual void destroy() { }

        bool isInstanceOf(const meta::Class& cls) const;
    };

    template<IsObject T>
    T *cast(Object& object) {
        if (object.isInstanceOf(T::getClass())) {
            return static_cast<T*>(&object);
        } else {
            return nullptr;
        }
    }

    /// @brief an entity in the game world
    REFLECT()
    class Entity : public Object {
        REFLECT_BODY(Entity)

        PROPERTY()
        math::float3 mPosition;

        PROPERTY()
        math::quatf mRotation;

        PROPERTY()
        math::float3 mScale;

    public:
        Entity();
    };

    REFLECT()
    class Component : public Object {
        REFLECT_BODY(Component)

    public:
        Component();
    };

    REFLECT()
    class ModelComponent : public Component {
        REFLECT_BODY(ModelComponent)

        PROPERTY()
        world::IndexOf<world::Model> mModel;

    public:
        ModelComponent();
    };

    enum class PhysicsLayer {
        eStatic,
        eDynamic,

        eCount
    };

    enum class PhysicsType {
        eStatic,
        eKinematic,
        eDynamic,

        eCount
    };

    REFLECT()
    class PhysicsComponent : public Component {
        REFLECT_BODY(PhysicsComponent)

        PROPERTY(name = "Activate On Create", category = "Physics")
        bool mActivateOnCreate;

        PROPERTY()
        PhysicsLayer mLayer;

        PROPERTY()
        PhysicsType mType;

    public:
        PhysicsComponent();
    };

    REFLECT()
    class CameraComponent : public Component {
        REFLECT_BODY(CameraComponent)

        PROPERTY()
        world::IndexOf<world::Camera> mCamera;

    public:
        CameraComponent();
    };

    REFLECT()
    class Character : public Entity {
        REFLECT_BODY(Character)

        PROPERTY()
        ModelComponent *mModelComponent;

        PROPERTY()
        PhysicsComponent *mPhysicsComponent;

    public:
        Character();
    };
}
