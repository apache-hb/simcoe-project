#pragma once

#include "editor/draw.hpp"

#include "imfilebrowser.h"

namespace sm::render {
    struct Context;
}

namespace sm::ed {
    class ViewportPanel;

    class AssetBrowserPanel final {
        [[maybe_unused]] ed::EditorContext &mContext;

        float mThumbnailSize = 64.0f;
        float mThumbnailPadding = 4.0f;

        ImGui::FileBrowser mFileBrowser { ImGuiFileBrowserFlags_CloseOnEsc | ImGuiFileBrowserFlags_MultipleSelection | ImGuiFileBrowserFlags_ConfirmOnEnter };

        // IEditorPanel
        void draw_content();

        void draw_images();
        void draw_models();
        void draw_materials();

    public:
        AssetBrowserPanel(ed::EditorContext& context);

        bool mOpen = true;
        void draw_window();
    };
}
