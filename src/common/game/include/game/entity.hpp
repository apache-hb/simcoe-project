#pragma once

#include "math/math.hpp"

#include "world/world.hpp"

#include "meta/meta.hpp"

namespace sm::game {
    /// @brief base object class
    /// opts in for reflection, serialization, and archetypes
    REFLECT()
    class Object {
        REFLECT_BODY(Object)

        uint32 mObjectId;

    public:
        virtual ~Object() = default;

        Object();

        virtual void update(float dt) { }
        virtual void birth() { }
        virtual void death() { }
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

        PROPERTY()
        bool mActivateOnBirth;

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
