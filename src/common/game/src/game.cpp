#include "stdafx.hpp" // IWYU pragma: export

#include "game/game.hpp"

using namespace sm;
using namespace sm::game;

void game::init(flecs::world& ecs) {
    ecs.entity("a")
        .set<Mesh, Cube>({1, 1, 1});

    ecs.observer<Mesh>()
        .event(flecs::OnSet)
        .each([](flecs::iter& it, size_t i, const Mesh& mesh) {
            if (it.entity(i).has<Mesh, Cube>()) {

            }
        });
}
