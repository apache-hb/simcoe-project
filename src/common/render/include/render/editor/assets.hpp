#pragma once

#include "render/editor/panel.hpp"

namespace sm::editor {
    class AssetBrowserPanel final : public IEditorPanel {
        // IEditorPanel
        void draw_content() override;

    public:
        AssetBrowserPanel();
    };
}
