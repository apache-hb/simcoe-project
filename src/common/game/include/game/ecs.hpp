#pragma once

#include "input/input.hpp"

#include <flecs.h>

namespace sm::game::ecs {
    void initCameraSystems(flecs::world& world);

    void updateCamera(flecs::entity camera, float dt, const input::InputState& state);
}
