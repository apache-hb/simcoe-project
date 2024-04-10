#pragma once

#include "math/math.hpp"

#include "meta/meta.hpp"

namespace sm::game {
    /// @brief base object class
    /// opts in for reflection, serialization, and archetypes
    REFLECT()
    class Object {
        REFLECT_BODY(Object)

        uint32 mObjectId;

    public:
        Object();
    };

    REFLECT()
    class Component {
        REFLECT_BODY(Component)

    public:
        Component();
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
}
