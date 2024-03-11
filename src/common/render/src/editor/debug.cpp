#include "render/editor/debug.hpp"

#include "stdafx.hpp"

using namespace sm;
using namespace sm::editor;

void DebugPanel::draw_content() {
    if (ImGui::Button("Not my fault!")) {
        SM_NEVER("Not my fault!");
    }
    ImGui::SetItemTooltip("Warning: This button will crash the editor!");
}

DebugPanel::DebugPanel()
    : IEditorPanel("Debug")
{ }
