#include "stdafx.hpp"

#include "game/entity.hpp"

using namespace sm;
using namespace sm::game;

Entity::Entity(ClassSetup& setup)
    : Object(setup)
{
    mPosition = 0.f;
    mRotation = math::quatf::identity();
    mScale = 1.f;
}
