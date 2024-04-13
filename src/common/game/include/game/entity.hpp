#pragma once

#include "meta/meta.hpp"
#include "meta/class.hpp"

#include "math/math.hpp"

#include "game/object.hpp"

#include "world/world.hpp"

#include "entity.reflect.h"

namespace sm::game {
    /// @brief an entity in the game world
    REFLECT()
    class Entity : public Object {
        REFLECT_BODY(Entity)

    public:
        PROPERTY(category = "Transform")
        math::float3 mPosition;

        PROPERTY(category = "Transform")
        math::quatf mRotation;

        PROPERTY(category = "Transform")
        math::float3 mScale;

        Entity(ClassSetup& setup);
    };

    REFLECT()
    class Component : public Entity {
        REFLECT_BODY(Component)

    public:
        Component();
    };

    REFLECT()
    class MeshComponent : public Component {
        REFLECT_BODY(MeshComponent)

    public:
        PROPERTY(name = "Model", category = "Mesh")
        world::IndexOf<world::Model> mModel;

        MeshComponent();
    };

    ENUM()
    enum class PhysicsLayer {
        eStatic,
        eDynamic,

        eCount
    };

    ENUM()
    enum class PhysicsType {
        eStatic,
        eKinematic,
        eDynamic,

        eCount
    };

    REFLECT()
    class PhysicsComponent : public Component {
        REFLECT_BODY(PhysicsComponent)

    public:
        PROPERTY(name = "Activate On Create", category = "Physics")
        bool mActivateOnCreate;

        PROPERTY(name = "Layer", category = "Physics")
        PhysicsLayer mLayer;

        PROPERTY(name = "Type", category = "Physics")
        PhysicsType mType;

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
        MeshComponent *mModelComponent;

        PROPERTY()
        PhysicsComponent *mPhysicsComponent;

    public:
        Character();
    };
}
