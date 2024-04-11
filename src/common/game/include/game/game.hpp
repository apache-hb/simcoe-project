#pragma once

#include "world/world.hpp"

#include "render/graph.hpp"

namespace sm::draw {
    class Camera;
}

namespace sm::game {
    struct IContext {
        virtual ~IContext() = default;
        virtual void tick(float dt) = 0;

        virtual void shutdown() = 0;

        virtual void set_camera(const draw::Camera& camera) = 0;

        static IContext& get();
    };

    IContext& init(world::World& world, const draw::Camera& camera);

    void physics_debug(
        graph::FrameGraph& graph,
        const draw::Camera& camera,
        graph::Handle target,
        graph::Handle depth);
}
