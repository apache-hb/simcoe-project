#pragma once

#include "meta/meta.hpp"

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
        Component(ClassSetup& setup);
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

    const char *getPhysicsLayerName(PhysicsLayer layer);
    const char *getPhysicsTypeName(PhysicsType type);

    REFLECT()
    class PhysicsComponent : public Component {
        REFLECT_BODY(PhysicsComponent)

    public:
        PROPERTY(category = "Physics")
        bool mActivateOnCreate;

        PROPERTY(category = "Physics")
        PhysicsLayer mLayer;

        PROPERTY(category = "Physics")
        PhysicsType mType;

        PhysicsComponent();
    };

    REFLECT()
    class CameraComponent : public Component {
        REFLECT_BODY(CameraComponent)

        math::degf mLookPitch = math::degf(0);
        math::degf mLookYaw = math::degf(0);

    public:
        CameraComponent(ClassSetup& setup);

        PROPERTY(category = "Camera")
        math::float3 mPosition;

        PROPERTY(category = "Camera")
        math::float3 mDirection;

        PROPERTY(category = "Camera", range = { 15.f, 140.f })
        float mFieldOfView;

        void update(float dt) override;

        math::float4x4 getModelMatrix() const;
        math::float4x4 getViewMatrix() const;
        math::float4x4 getProjectionMatrix(float aspect) const;
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
