#pragma once

#include "render/render.hpp"

namespace sm::render {
    struct Context;
}

namespace sm::ed {
    class FeatureSupportPanel final {
        render::Context &mContext;

        void draw_content();

    public:
        FeatureSupportPanel(render::Context &context);

        bool mOpen = true;
        void draw_window();
    };
}