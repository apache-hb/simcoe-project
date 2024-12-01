#pragma once

#if 0
#include "meta/class.hpp"

#include "character.meta.h"

namespace sm::project {
    REFLECT()
    class Character : game::Entity {
        REFLECT_BODY(Character)

        PROPERTY(name = "Model", category = "Mesh", transient = true)
        world::IndexOf<world::Model> mModel;

    public:
        PROPERTY(category = "Character")
        float mHeight;

        PROPERTY(category = "Character")
        float mRadius;

        Character(game::ClassSetup& setup);

        void create() override;
        void destroy() override;
        void update(float dt) override;
    };
}
#endif
