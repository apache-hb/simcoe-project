#pragma once

#include "game/game.hpp"

namespace sm::ed {
    struct EditorContext;

    struct PhysicsDebug {
        ed::EditorContext& context;
        bool isOpen = true;

        world::IndexOf<world::Node> pickedNode = 0;
        bool dynamicObject = false;
        bool activateOnCreate = false;

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

        void addPhysicsBody(world::IndexOf<world::Node> node, game::PhysicsBody&& body);

        void createPhysicsBody(world::IndexOf<world::Node> node, bool sphere, bool dynamic);

        void update();
    };
}
