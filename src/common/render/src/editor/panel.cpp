#include "render/editor/panel.hpp"

#include "imgui/imgui.h"

using namespace sm;
using namespace sm::editor;

void IEditorPanel::draw_menu_item(const char *shortcut) {
    ImGui::MenuItem(mTitle.c_str(), shortcut, &mOpen);
}

void IEditorPanel::draw_window() {
    if (!mOpen) return;

    if (ImGui::Begin(mTitle.c_str(), &mOpen)) {
        draw_content();
    }
    ImGui::End();
}

void IEditorPanel::draw_section() {
    ImGui::SeparatorText(mTitle.c_str());
    draw_content();
}
