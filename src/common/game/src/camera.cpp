#include "stdafx.hpp"

#include "game/ecs.hpp"
#include "world/ecs.hpp"

#include "input/delta.hpp"

#include "math/format.hpp"

using namespace sm;
using namespace sm::game;
using namespace sm::math::literals;

static constexpr input::ButtonAxis kMoveForward = {input::Button::eW, input::Button::eS};
static constexpr input::ButtonAxis kMoveStrafe  = {input::Button::eD, input::Button::eA};
static constexpr input::ButtonAxis kMoveUp      = {input::Button::eE, input::Button::eQ};

struct CameraController {
    float speed = 3.f;
    float mouseSensitivity = 25.f;

    math::degf lookPitch = math::degf(0);
    math::degf lookYaw = math::degf(0);

    input::DeltaAxis<math::float2> mouseDelta;
};

static constexpr math::float3 makeDirectionVector(math::degf pitch, math::degf yaw) {
    math::float3 front = {{
        .roll = math::cos(pitch) * math::cos(yaw),
        .pitch = math::sin(pitch),
        .yaw = math::cos(pitch) * math::sin(yaw)
    }};

    return front.normalized();
}

void ecs::initCameraSystems(flecs::world& world) {
    world.component<world::ecs::Position>().member<math::float3>("position");
    world.component<world::ecs::Rotation>().member<math::quatf>("rotation");
    world.component<world::ecs::Direction>().member<math::float3>("direction");

    world.component<world::ecs::AABB>()
        .member<math::float3>("min")
        .member<math::float3>("max");

    world.component<CameraController>()
        .member<float>("speed")
        .member<float>("mouseSensitivity")
        .member<math::degf>("lookPitch")
        .member<math::degf>("lookYaw")
        .member<input::DeltaAxis<math::float2>>("mouseDelta");

    world.observer()
        .with<world::ecs::Camera>()
        .event(flecs::OnAdd)
        .event(flecs::OnRemove)
        .each([](flecs::iter& it, size_t i) {
            flecs::entity entity = it.entity(i);

            if (it.event() == flecs::OnAdd) {
                const world::ecs::Direction *dir = entity.get<world::ecs::Direction>();
                auto [dx, dy, dz] = dir->direction;
                math::degf pitch = math::degf(std::asin(dy));
                math::degf yaw = math::degf(std::atan2(dx, dz));

                pitch = math::clamp(pitch, -89._deg, 89._deg);

                math::float3 front = makeDirectionVector(pitch, yaw);

                entity.set([&](CameraController& controller, world::ecs::Direction& dir) {
                    dir.direction = front;

                    controller.lookPitch = pitch;
                    controller.lookYaw = yaw;
                });
            } else {
                entity.remove<CameraController>();
            }
        });
}

void ecs::updateCamera(flecs::entity camera, float dt, const input::InputState& state) {
    // get components
    const world::ecs::Position *pos = camera.get<world::ecs::Position>();
    CameraController controller = *camera.get<CameraController>();

    // read input
    math::float2 mouseInput = state.axis2d(input::Axis::eMouseX, input::Axis::eMouseY);
    math::float2 mouse = controller.mouseDelta.getNewDelta(mouseInput * 10) * controller.mouseSensitivity * dt;
    math::float3 move = state.button_axis3d(kMoveStrafe, kMoveUp, kMoveForward) * controller.speed * dt;

    // orientation

    controller.lookYaw -= math::degf(mouse.x);
    controller.lookPitch += math::degf(mouse.y);

    controller.lookPitch = math::clamp(controller.lookPitch, -89._deg, 89._deg);

    const math::float3 front = makeDirectionVector(controller.lookPitch, controller.lookYaw);


    // movement

    const math::float3 position = [&] {
        math::float3 tmp = pos->position;
        tmp -= front * move.z;
        tmp -= math::float3::cross(front, world::kVectorUp).normalized() * move.x;
        tmp.y -= move.y;
        return tmp;
    }();

    // write back
    camera.set([=](world::ecs::Direction& dir, world::ecs::Position& pos, CameraController& info) {
        dir.direction = front;
        pos.position = position;
        info = controller;
    });
}
