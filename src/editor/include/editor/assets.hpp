#pragma once

#include "editor/panel.hpp"

#include "imfilebrowser.h"

namespace sm::render {
    struct Context;
}

namespace sm::ed {
    class ViewportPanel;

    class AssetBrowserPanel final : public IEditorPanel {
        render::Context &mContext;
        ViewportPanel &mViewport;

        float mThumbnailSize = 64.0f;
        float mThumbnailPadding = 4.0f;

        ImGui::FileBrowser mFileBrowser { ImGuiFileBrowserFlags_CloseOnEsc | ImGuiFileBrowserFlags_MultipleSelection | ImGuiFileBrowserFlags_ConfirmOnEnter };

        // IEditorPanel
        void draw_content() override;

        void draw_images();
        void draw_models();
        void draw_materials();

    public:
        AssetBrowserPanel(render::Context& context, ViewportPanel& viewport);
    };
}
