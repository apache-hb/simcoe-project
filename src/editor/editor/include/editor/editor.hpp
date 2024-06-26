#pragma once

#include "editor/panels/features.hpp"
#include "editor/panels/graph.hpp"
#include "editor/panels/inspector.hpp"
#include "editor/panels/logger.hpp"
#include "editor/panels/config.hpp"
#include "editor/panels/scene.hpp"
#include "editor/panels/debug.hpp"
#include "editor/panels/assets.hpp"
#include "editor/panels/pix.hpp"
#include "editor/panels/physics.hpp"
#include "editor/draw.hpp"

#include "imfilebrowser.h"

namespace sm::render {
    struct IDeviceContext;
}

namespace sm::ed {
    world::IndexOf<world::Image> loadImageInfo(world::World& world, const fs::path& path);

    class Editor {
        EditorContext &mContext;

        LoggerPanel& mLogger;
        RenderConfig mConfig;
        ScenePanel mScene;
        Inspector mInspector;
        FeatureSupportPanel mFeatureSupport;
        DebugPanel mDebug;
        AssetBrowser mAssetBrowser;
        GraphPanel mGraph;
        PixPanel mPix;
        PhysicsDebug mPhysicsDebug;

        bool mShowImGuiDemo = false;
        bool mShowImPlotDemo = false;

        enum Theme {
            eDark,
            eLight,
            eClassic
        } mTheme = Theme::eDark;

        // ImGui::FileBrowser mSaveLevelDialog { ImGuiFileBrowserFlags_CreateNewDir | ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_CloseOnEsc | ImGuiFileBrowserFlags_ConfirmOnEnter };
        ImGui::FileBrowser mOpenLevelDialog {
            ImGuiFileBrowserFlags_CloseOnEsc |
            ImGuiFileBrowserFlags_ConfirmOnEnter
        };
        ImGui::FileBrowser mImportDialog {
            ImGuiFileBrowserFlags_CloseOnEsc |
            ImGuiFileBrowserFlags_ConfirmOnEnter |
            ImGuiFileBrowserFlags_MultipleSelection
        };

        world::IndexOf<world::Node> get_best_node();

        void importGltf(const fs::path& path);

        void importBlockCompressedImage(const fs::path& path);

        void importImage(const fs::path& path);

        void importFile(const fs::path& path);

        void importLevel(const fs::path& path);

        void draw_mainmenu();
        void draw_dockspace();

        void draw_create_popup();

        std::string mErrorMessage;
        void alert_error(std::string message);
        void draw_error_modal();

    public:
        Editor(ed::EditorContext &context);

        void begin_frame();
        void end_frame();

        void draw();

        render::IDeviceContext& getContext() { return mContext; }
        // draw::Camera& get_camera() { return mContext.get_active_camera(); }

        void addPhysicsBody(world::IndexOf<world::Node> node, game::PhysicsBody&& body) {
            mPhysicsDebug.addPhysicsBody(node, std::move(body));
        }

        void update() {
            mPhysicsDebug.update();
        }
    };
}
