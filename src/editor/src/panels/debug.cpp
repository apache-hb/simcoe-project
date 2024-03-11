#include "stdafx.hpp"

#include "editor/debug.hpp"

using namespace sm;
using namespace sm::ed;

void DebugPanel::draw_content() {
    if (ImGui::Button("Not my fault!")) {
        SM_NEVER("Not my fault!");
    }
    ImGui::SetItemTooltip("Warning: This button will crash the editor!");
}

DebugPanel::DebugPanel()
    : IEditorPanel("Debug")
{ }
