#include "render/editor/editor.hpp"

#include "imgui/imgui.h"
#include "implot.h"

using namespace sm;
using namespace sm::editor;

class ImGuiDemoPanel final : public IEditorPanel {
    // IEditorPanel
    void draw_content() override { }

    bool draw_window() override {
        if (bool open = is_open()) {
            ImGui::ShowDemoWindow(get_open());
        }

        return is_open();
    }

public:
    ImGuiDemoPanel() : IEditorPanel("ImGui Demo") { }
};

class ImPlotDemoPanel final : public IEditorPanel {
    // IEditorPanel
    void draw_content() override { }

    bool draw_window() override {
        if (bool open = is_open()) {
            ImPlot::ShowDemoWindow(get_open());
        }

        return is_open();
    }
public:
    ImPlotDemoPanel() : IEditorPanel("ImPlot Demo") { }
};

template<typename F>
class StyleMenuItem final : public IEditorPanel {
    F mStyle;

    // IEditorPanel
    void draw_content() override { }
    bool draw_window() override { return false; }

    bool draw_menu_item(const char *shortcut) override {
        bool result = IEditorPanel::draw_menu_item(shortcut);
        if (result)
            mStyle();

        return result;
    }
public:
    StyleMenuItem(sm::StringView title, F style)
        : IEditorPanel(title)
        , mStyle(style)
    { }
};

Editor::Editor(render::Context& context)
    : mContext(context)
    , mLogger(LoggerPanel::get())
    , mConfig(context)
    , mViewport(context)
    , mScene(context, mViewport)
    , mInspector(context, mViewport)
    , mFeatureSupport(context)
    , mAssetBrowser(context)
    , mGraph(context)
    , mPix(context)
{
    mOpenLevelDialog.SetTitle("Open Level");
    mSaveLevelDialog.SetTitle("Save Level");
    mSaveLevelDialog.SetTypeFilters({ ".bin" });
    mSaveLevelDialog.SetInputName("level.bin");

    EditorMenu view_menu = {
        .name = "View",
        .header = { &mLogger, &mScene, &mViewport, &mAssetBrowser, &mInspector },
        .sections = {
            MenuSection {
                .name = "Debugging",
                .panels = { &mDebug, &mPix, &mFeatureSupport }
            },
            MenuSection {
                .name = "Demo Windows",
                .panels = { new ImGuiDemoPanel(), new ImPlotDemoPanel() }
            }
        }
    };

    EditorMenu settings_menu = {
        .name = "Settings",
        .header = { &mConfig },
        .sections = {
            MenuSection {
                .name = "Style",
                .panels = {
                    new StyleMenuItem("Dark", [] { ImGui::StyleColorsDark(); ImPlot::StyleColorsDark(); }),
                    new StyleMenuItem("Light", [] { ImGui::StyleColorsLight(); ImPlot::StyleColorsLight(); }),
                    new StyleMenuItem("Classic", [] { ImGui::StyleColorsClassic(); ImPlot::StyleColorsClassic(); })
                }
            }
        }
    };

    mMenus.push_back(view_menu);
    mMenus.push_back(settings_menu);
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

        for (const auto& [name, header, sections] : mMenus) {
            if (ImGui::BeginMenu(name)) {
                for (IEditorPanel *panel : header) {
                    panel->draw_menu_item();
                }

                for (auto& [section_name, panels] : sections) {
                    ImGui::SeparatorText(section_name);
                    for (IEditorPanel *panel : panels) {
                        panel->draw_menu_item();
                    }
                }
                ImGui::EndMenu();
            }
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

    for (auto& [_, header, sections] : mMenus) {
        for (IEditorPanel *panel : header) {
            panel->draw_window();
        }

        for (auto& [_, panels] : sections) {
            for (IEditorPanel *panel : panels) {
                panel->draw_window();
            }
        }
    }

    mOpenLevelDialog.Display();
    mSaveLevelDialog.Display();
}
