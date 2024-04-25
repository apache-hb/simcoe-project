#include "stdafx.hpp"

#include "world/ecs.hpp"

using namespace sm;
using namespace sm::world;

math::float4x4 ecs::getViewMatrix(ecs::Position position, ecs::Direction direction) {
    return math::float4x4::lookToRH(position.position, direction.direction, world::kVectorUp);
}

math::float4x4 ecs::Camera::getProjectionMatrix() const {
    return math::float4x4::perspectiveRH(fov, getAspectRatio(), 0.1f, 1000.0f);
}
