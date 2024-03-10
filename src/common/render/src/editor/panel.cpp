#include "render/editor/panel.hpp"

#include "imgui/imgui.h"

using namespace sm;
using namespace sm::editor;

void IEditorPanel::set_window_flags(ImGuiWindowFlags flags) { mFlags = flags; }
ImGuiWindowFlags IEditorPanel::get_window_flags() const { return mFlags; }

void IEditorPanel::set_shortcut(const char *shortcut) { mShortcut = shortcut; }
const char *IEditorPanel::get_shortcut() const { return mShortcut; }

void IEditorPanel::set_title(sm::StringView title) { mTitle = title; }
const char *IEditorPanel::get_title() const { return mTitle.c_str(); }

void IEditorPanel::set_open(bool open) { mOpen = open; }
bool IEditorPanel::is_open() const { return mOpen; }
bool *IEditorPanel::get_open() { return &mOpen; }

bool IEditorPanel::draw_menu_item(const char *shortcut) {
    const char *hotkey = (shortcut == nullptr) ? get_shortcut() : shortcut;
    return ImGui::MenuItem(get_title(), hotkey, &mOpen);
}

bool IEditorPanel::draw_window() {
    if (mOpen) {
        if (ImGui::Begin(get_title(), &mOpen, mFlags)) {
            draw_content();
        }
        ImGui::End();
    }

    return mOpen;
}

void IEditorPanel::draw_section() {
    ImGui::SeparatorText(get_title());
    draw_content();
}
