#pragma once

#include "render/editor/panel.hpp"

namespace sm::render {
    struct Context;
}

namespace sm::editor {
    class FeatureSupportPanel final : public IEditorPanel {
        render::Context &mContext;

        void draw_feature(const char *name, bool value);

        // IEditorPanel
        void draw_content() override;

    public:
        FeatureSupportPanel(render::Context &context);
    };
}