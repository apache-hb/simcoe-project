#include "render/editor/editor.hpp"

#include "imgui/imgui.h"
#include "implot.h"

using namespace sm;
using namespace sm::editor;

Editor::Editor(render::Context& context)
    : mContext(context)
    , mLogger(LoggerPanel::get())
    , mConfig(context)
{ }

void Editor::draw_mainmenu() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("New Level");
            ImGui::MenuItem("Open");
            ImGui::MenuItem("Save");
            ImGui::MenuItem("Save As");
            ImGui::MenuItem("Exit");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            mLogger.draw_menu_item();
            mConfig.draw_menu_item();

            ImGui::SeparatorText("Demo Windows");
            ImGui::MenuItem("ImGui Demo", nullptr, &mShowDemo);
            ImGui::MenuItem("ImPlot Demo", nullptr, &mShowPlotDemo);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Style")) {
            if (ImGui::MenuItem("Dark")) {
                ImGui::StyleColorsDark();
                ImPlot::StyleColorsDark();
            }

            if (ImGui::MenuItem("Light")) {
                ImGui::StyleColorsLight();
                ImPlot::StyleColorsLight();
            }

            if (ImGui::MenuItem("Classic")) {
                ImGui::StyleColorsClassic();
                ImPlot::StyleColorsClassic();
            }

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

void Editor::draw() {
    draw_mainmenu();
    draw_dockspace();

    if (mShowDemo) {
        ImGui::ShowDemoWindow(&mShowDemo);
    }

    if (mShowPlotDemo) {
        ImPlot::ShowDemoWindow(&mShowPlotDemo);
    }

    mLogger.draw_window();
    mConfig.draw_window();
}
