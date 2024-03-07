#pragma once

#include "render/editor/panel.hpp"
#include "render/editor/scene.hpp"

namespace sm::editor {
    class InspectorPanel final : public IEditorPanel {
        render::Context &mContext;
        ViewportPanel &mViewport;

        // IEditorPanel
        void draw_content() override;

    public:
        InspectorPanel(render::Context &context, ViewportPanel &viewport);
    };
}
