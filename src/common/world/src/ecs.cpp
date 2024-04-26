#include "stdafx.hpp"

#include "world/ecs.hpp"

using namespace sm;
using namespace sm::world;

void ecs::initSystems(flecs::world& world) {
    world.observer<const Position>()
        .event(flecs::Monitor)
        .term_at(1).second<Local>()
        .each([](flecs::iter& it, size_t i, const Position&) {
            flecs::entity entity = it.entity(i);
            if (it.event() == flecs::OnAdd) {
                entity.add<Position, World>();
            } else if (it.event() == flecs::OnRemove) {
                entity.remove<Position, World>();
            }
        });

    world.system<const Position, const Position, Position>("Update Entity Positions")
        .kind(flecs::OnUpdate)
        .term_at(1).second<Local>()
        .term_at(2).second<World>()
        .term_at(3).second<World>()

        // select the parent position
        .term_at(2)
            // search breadth-first to find the parent
            .parent().cascade()
            // make the term optional so we also match the root
            .optional()
        .iter([](flecs::iter& it, const Position* local, const Position* parent, Position* result) {
            for (auto i : it) {
                result[i].position = local[i].position;
                if (parent) {
                    result[i].position += parent->position;
                }
            }
        });
}

math::float4x4 ecs::getViewMatrix(ecs::Position position, ecs::Direction direction) {
    return math::float4x4::lookToRH(position.position, direction.direction, world::kVectorUp);
}

math::float4x4 ecs::Camera::getProjectionMatrix() const {
    return math::float4x4::perspectiveRH(fov, getAspectRatio(), 0.1f, 100.0f);
}
