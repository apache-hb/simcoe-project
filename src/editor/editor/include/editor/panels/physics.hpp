#pragma once

#include "game/game.hpp"

namespace sm::ed {
    struct EditorContext;

    struct PhysicsDebug {
        ed::EditorContext& context;
        bool isOpen = true;

        struct PhysicsObjectData {
            world::IndexOf<world::Node> node;
            game::PhysicsBody body;
            bool debugDraw = false;
        };

        sm::Vector<PhysicsObjectData> bodies;

        PhysicsDebug(ed::EditorContext& context)
            : context(context)
        { }

        void draw_window();

        void addPhysicsBody(world::IndexOf<world::Node> node, game::PhysicsBody&& body) {
            bodies.emplace_back(PhysicsObjectData { node, std::move(body) });
        }

        void update();
    };
}
