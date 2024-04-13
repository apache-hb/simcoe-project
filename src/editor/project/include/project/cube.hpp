#pragma once

#include "meta/meta.hpp"

#include "game/entity.hpp"

#include "cube.meta.h"

namespace sm::project {
    REFLECT()
    class Cube : public game::Entity {
        REFLECT_BODY(Cube)

        PROPERTY(name = "Model", category = "Mesh", transient = true)
        world::IndexOf<world::Model> mModel;

    public:
        PROPERTY(category = "Mesh")
        float mWidth;

        PROPERTY(category = "Mesh")
        float mHeight;

        PROPERTY(category = "Mesh")
        float mDepth;

        Cube(game::ClassSetup& setup);

        void create() override;
        void destroy() override;
        void update(float dt) override;
    };
}
