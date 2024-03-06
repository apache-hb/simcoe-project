#pragma once

#include "render/editor/panel.hpp"

namespace sm::editor {
    class DebugPanel final : public IEditorPanel {
        // IEditorPanel
        void draw_content() override;
    public:
        DebugPanel();
    };
}
