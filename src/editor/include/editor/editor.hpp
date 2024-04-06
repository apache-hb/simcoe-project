#pragma once

#include "editor/panels/features.hpp"
#include "editor/panels/graph.hpp"
#include "editor/panels/inspector.hpp"
#include "editor/panels/logger.hpp"
#include "editor/panels/config.hpp"
#include "editor/panels/scene.hpp"
#include "editor/panels/debug.hpp"
#include "editor/panels/assets.hpp"
#include "editor/panels/viewport.hpp"
#include "editor/panels/pix.hpp"
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

        world::IndexOf<world::Node> get_picked_node();

        void draw_mainmenu();
        void draw_dockspace();

        void draw_create_popup();

    public:
        Editor(ed::EditorContext &context);

        void begin_frame();
        void end_frame();

        void draw();

        render::Context& get_context() { return mContext; }
        draw::Camera& get_camera() { return *mContext.cameras[mContext.active].camera.get(); }
    };
}
