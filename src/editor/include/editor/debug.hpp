#pragma once

namespace sm::ed {
    class DebugPanel final {
        // IEditorPanel
        void draw_content();
    public:
        DebugPanel() = default;

        bool mOpen = true;
        void draw_window();
    };
}
