#include "stdafx.hpp"

#include "game/entity.hpp"

using namespace sm;
using namespace sm::game;

using namespace sm::math;

const char *game::getPhysicsLayerName(PhysicsLayer layer) {
    switch (layer) {
    case PhysicsLayer::eStatic:
        return "Static";
    case PhysicsLayer::eDynamic:
        return "Dynamic";
    default:
        return "Unknown";
    }
}

const char *game::getPhysicsTypeName(PhysicsType type) {
    switch (type) {
    case PhysicsType::eStatic:
        return "Static";
    case PhysicsType::eKinematic:
        return "Kinematic";
    case PhysicsType::eDynamic:
        return "Dynamic";
    default:
        return "Unknown";
    }
}

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
    mFieldOfView = math::degf(90.f).get_degrees();
}

void CameraComponent::update(float dt) {

}
