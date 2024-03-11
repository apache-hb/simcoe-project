#pragma once

#include "editor/panel.hpp"

namespace sm::ed {
    class DebugPanel final : public IEditorPanel {
        // IEditorPanel
        void draw_content() override;
    public:
        DebugPanel();
    };
}
