#pragma once

#include "editor/features.hpp"
#include "editor/graph.hpp"
#include "editor/inspector.hpp"
#include "editor/logger.hpp"
#include "editor/config.hpp"
#include "editor/scene.hpp"
#include "editor/debug.hpp"
#include "editor/assets.hpp"
#include "editor/viewport.hpp"
#include "editor/pix.hpp"
#include "editor/draw.hpp"

#include "imfilebrowser.h"

namespace sm::render {
    struct Context;
}

namespace sm::ed {
    class Editor {
        EditorContext &mContext;

        LoggerPanel& mLogger;
        RenderConfig mConfig;
        sm::Vector<ViewportPanel> mViewports;
        ScenePanel mScene;
        InspectorPanel mInspector;
        FeatureSupportPanel mFeatureSupport;
        DebugPanel mDebug;
        AssetBrowserPanel mAssetBrowser;
        GraphPanel mGraph;
        PixPanel mPix;

        bool mShowImGuiDemo = false;
        bool mShowImPlotDemo = false;

        enum Theme {
            eDark,
            eLight,
            eClassic
        } mTheme = Theme::eDark;

        ImGui::FileBrowser mSaveLevelDialog { ImGuiFileBrowserFlags_CreateNewDir | ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_CloseOnEsc | ImGuiFileBrowserFlags_ConfirmOnEnter };
        ImGui::FileBrowser mOpenLevelDialog { ImGuiFileBrowserFlags_CloseOnEsc | ImGuiFileBrowserFlags_ConfirmOnEnter };

        void draw_mainmenu();
        void draw_dockspace();

    public:
        Editor(ed::EditorContext &context);

        void begin_frame();
        void end_frame();

        void draw();

        render::Context& get_context() { return mContext; }
        draw::Camera& get_camera() { return *mContext.cameras[mContext.active].camera.get(); }
    };
}
