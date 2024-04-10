#pragma once

#include "world/world.hpp"

namespace sm::game {
    struct IContext {
        virtual ~IContext() = default;
        virtual void tick(float dt) = 0;

        virtual void shutdown() = 0;

        static IContext& get();
    };

    IContext& init(world::World& world);
}
