#include "stdafx.hpp"

#include "editor/editor.hpp"
#include "editor/mygui.hpp"

using namespace sm;
using namespace sm::ed;

Editor::Editor(ed::EditorContext& context)
    : mContext(context)
    , mLogger(LoggerPanel::get())
    , mConfig(context)
    , mScene(context)
    , mInspector(context)
    , mFeatureSupport(context)
    , mAssetBrowser(context)
    , mGraph(context)
    , mPix(context)
{
    mOpenLevelDialog.SetTitle("Open Level");
    mSaveLevelDialog.SetTitle("Save Level");
    mSaveLevelDialog.SetTypeFilters({ ".bin" });
    mSaveLevelDialog.SetInputName("level.bin");

    for (size_t i = 0; i < mContext.cameras.size(); i++) {
        mViewports.emplace_back(mContext, i);
    }
}

void Editor::draw_mainmenu() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("New Level");
            ImGui::Separator();

            if (ImGui::MenuItem("Open")) {
                mOpenLevelDialog.Open();
            }
            ImGui::Separator();

            // TODO: implement save and save as
            if (ImGui::MenuItem("Save")) {
                mSaveLevelDialog.Open();
            }

            if (ImGui::MenuItem("Save As")) {
                mSaveLevelDialog.Open();
            }
            ImGui::Separator();

            ImGui::MenuItem("Exit");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Logger", nullptr, &mLogger.mOpen);
            ImGui::MenuItem("Scene Tree", nullptr, &mScene.mOpen);
            ImGui::MenuItem("Inspector", nullptr, &mInspector.mOpen);
            ImGui::MenuItem("Asset Browser", nullptr, &mAssetBrowser.mOpen);

            ImGui::SeparatorText("Debugging");
            ImGui::MenuItem("Debug", nullptr, &mDebug.mOpen);
            mPix.draw_menu_item(nullptr);
            ImGui::MenuItem("Feature Support", nullptr, &mFeatureSupport.mOpen);

            ImGui::Separator();
            if (ImGui::BeginMenu("Theme")) {
                if (ImGui::MenuItem("Dark", nullptr, mTheme == Theme::eDark)) {
                    ImGui::StyleColorsDark();
                    ImPlot::StyleColorsDark();
                    mTheme = Theme::eDark;
                }

                if (ImGui::MenuItem("Light", nullptr, mTheme == Theme::eLight)) {
                    ImGui::StyleColorsLight();
                    ImPlot::StyleColorsLight();
                    mTheme = Theme::eLight;
                }

                if (ImGui::MenuItem("Classic", nullptr, mTheme == Theme::eClassic)) {
                    ImGui::StyleColorsClassic();
                    ImPlot::StyleColorsClassic();
                    mTheme = Theme::eClassic;
                }
                ImGui::EndMenu();
            }

            ImGui::SeparatorText("Viewports");
            for (ViewportPanel &viewport : mViewports) {
                ImGui::MenuItem(viewport.get_title(), nullptr, &viewport.mOpen);
            }
            if (ImGui::SmallButton("Add Viewport")) {
                size_t index = mContext.add_camera();
                mViewports.emplace_back(mContext, index);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("ImGui Demo", nullptr, &mShowImGuiDemo);
            ImGui::MenuItem("ImPlot Demo", nullptr, &mShowImPlotDemo);

            ImGui::Separator();

            ImGui::MenuItem("Settings", nullptr, &mConfig.mOpen);
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

static constexpr ImGuiWindowFlags kDockFlags
    = ImGuiWindowFlags_NoDocking
    | ImGuiWindowFlags_NoTitleBar
    | ImGuiWindowFlags_NoCollapse
    | ImGuiWindowFlags_NoResize
    | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoBringToFrontOnFocus
    | ImGuiWindowFlags_NoNavFocus;

void Editor::draw_dockspace() {
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    if (ImGui::Begin("Editor", nullptr, kDockFlags)) {
        ImGui::DockSpace(ImGui::GetID("EditorDock"), ImVec2(0.f, 0.f), ImGuiDockNodeFlags_None);
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
}

void Editor::begin_frame() {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    // TODO: get this from the current camera
    ImGuizmo::SetOrthographic(false);
}

void Editor::end_frame() {
    ImGui::Render();

    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

world::IndexOf<world::Node> Editor::get_picked_node() {
    if (!mContext.selected.has_value())
        return mContext.get_scene().root;

    auto index = world::get<world::Node>(*mContext.selected);
    if (index == world::kInvalidIndex)
        return mContext.get_scene().root;

    return index;
}

void Editor::draw_create_popup() {
    if (ImGui::IsKeyChordPressed(ImGuiMod_Shift | ImGuiKey_A)) {
        ImGui::OpenPopup("Create");
    }

    if (MyGui::BeginPopupWindow("Create", ImGuiWindowFlags_NoMove)) {
        if (ImGui::BeginMenu("Object")) {
            if (ImGui::MenuItem("Empty")) {
                auto root = get_picked_node();

                auto added = mContext.mWorld.add(world::Node { .name = "New Empty", .transform = world::default_transform() });
                mContext.mWorld.get(root).children.push_back(added);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Primitive")) {
            ImGui::MenuItem("Cube");
            ImGui::MenuItem("Sphere");
            ImGui::MenuItem("Cylinder");
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }
}

void Editor::draw() {
    draw_mainmenu();
    draw_dockspace();
    draw_create_popup();

    mLogger.draw_window();
    mScene.draw_window();
    mInspector.draw_window();
    mFeatureSupport.draw_window();
    mDebug.draw_window();
    mAssetBrowser.draw_window();
    mGraph.draw_window();
    mPix.draw_window();
    mConfig.draw_window();

    for (ViewportPanel &viewport : mViewports) {
        viewport.draw_window();
    }

    mOpenLevelDialog.Display();
    mSaveLevelDialog.Display();

    if (mShowImGuiDemo)
        ImGui::ShowDemoWindow(&mShowImGuiDemo);

    if (mShowImPlotDemo)
        ImPlot::ShowDemoWindow(&mShowImPlotDemo);
}
