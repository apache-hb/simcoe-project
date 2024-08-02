#pragma once

#include "render/core/render.hpp"

namespace sm::render {
    struct IDeviceContext;
}

namespace sm::ed {
    class FeatureSupportPanel final {
        render::IDeviceContext &mContext;

        void draw_content();

    public:
        FeatureSupportPanel(render::IDeviceContext &context);

        bool mOpen = true;
        void draw_window();
    };
}