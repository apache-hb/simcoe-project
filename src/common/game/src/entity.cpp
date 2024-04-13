#include "stdafx.hpp"

#include "game/entity.hpp"

using namespace sm;
using namespace sm::game;

using namespace sm::math;

Entity::Entity(ClassSetup& setup)
    : Object(setup)
{
    mPosition = 0.f;
    mRotation = math::quatf::identity();
    mScale = 1.f;
}

Component::Component(ClassSetup& setup)
    : Entity(setup)
{

}

CameraComponent::CameraComponent(ClassSetup& setup)
    : Component(setup)
{
    mPosition = 0.f;
    mDirection = world::kVectorForward;
    mFieldOfView = 90._deg;
}

void CameraComponent::update(float dt) {

}
