#pragma once

#include "editor/panel.hpp"

namespace sm::render {
    struct Context;
}

namespace sm::ed {
    class FeatureSupportPanel final : public IEditorPanel {
        render::Context &mContext;

        // IEditorPanel
        void draw_content() override;

    public:
        FeatureSupportPanel(render::Context &context);
    };
}