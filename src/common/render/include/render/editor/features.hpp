#pragma once

#include "render/editor/panel.hpp"

#include "render/object.hpp"

namespace sm::render {
    struct Context;
}

namespace sm::editor {
    class FeatureSupportPanel final : public IEditorPanel {
        render::Context &mContext;

        HMODULE mModule;
        UINT mVersion = 0;

        // IEditorPanel
        void draw_content() override;

    public:
        FeatureSupportPanel(render::Context &context);
    };
}