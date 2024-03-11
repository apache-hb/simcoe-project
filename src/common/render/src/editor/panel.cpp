#include "stdafx.hpp"

#include "render/editor/panel.hpp"

#include "render/mygui.hpp"

using namespace sm;
using namespace sm::editor;
using namespace sm::math;

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

void editor::edit_transform(world::Transform& transform) {
	auto& [position, rotation, scale] = transform;
	ImGui::DragFloat3("Translation", position.data());
    MyGui::DragAngle3("Rotation", &rotation, 1._deg, -180._deg, 180._deg);
	ImGui::DragFloat3("Scale", scale.data());
}
