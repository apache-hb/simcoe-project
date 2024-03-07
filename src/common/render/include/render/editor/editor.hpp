#pragma once

#include "render/editor/features.hpp"
#include "render/editor/inspector.hpp"
#include "render/editor/logger.hpp"
#include "render/editor/config.hpp"
#include "render/editor/scene.hpp"
#include "render/editor/debug.hpp"
#include "render/editor/assets.hpp"
#include "render/editor/viewport.hpp"

#include "imfilebrowser.h"

namespace sm::render {
    struct Context;
}

namespace sm::editor {
    class Editor {
        render::Context &mContext;

        LoggerPanel& mLogger;
        RenderConfig mConfig;
        ViewportPanel mViewport;
        ScenePanel mScene;
        InspectorPanel mInspector;
        FeatureSupportPanel mFeatureSupport;
        DebugPanel mDebug;
        AssetBrowserPanel mAssetBrowser;

        ImGui::FileBrowser mSaveLevelDialog { ImGuiFileBrowserFlags_CreateNewDir | ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_CloseOnEsc | ImGuiFileBrowserFlags_ConfirmOnEnter };
        ImGui::FileBrowser mOpenLevelDialog { ImGuiFileBrowserFlags_CloseOnEsc | ImGuiFileBrowserFlags_ConfirmOnEnter };

        bool mShowDemo = true;
        bool mShowPlotDemo = true;

        void draw_mainmenu();
        void draw_dockspace();

    public:
        Editor(render::Context &context);

        void draw();

        render::Context& get_context() { return mContext; }
    };
}
