#include "render/editor/inspector.hpp"

#include "render/editor/viewport.hpp"

#include "render/render.hpp"

using namespace sm;
using namespace sm::editor;
using namespace sm::math;

void InspectorPanel::draw_content() {
    auto selected = mViewport.get_selected();
    switch (selected.type) {
    case ItemType::eNone:
        ImGui::Text("Nothing selected");
        return;
    case ItemType::eNode:
        ImGui::Text("Node %s selected", mContext.mWorld.info.nodes[selected.index].name.data());
        break;
    case ItemType::eMesh:
        ImGui::Text("Mesh %s selected", mContext.mWorld.info.objects[selected.index].name.data());
        break;
    }

    if (selected.type != ItemType::eNode) return;

    auto& node = mContext.mWorld.info.nodes[selected.index];

    ImGui::Text("%zu children, %zu objects", node.children.size(), node.objects.size());

    ImGui::SeparatorText("Transform");
    edit_transform(node.transform);

    ImGui::SeparatorText("Gizmo");
    mViewport.gizmo_settings_panel();
}

InspectorPanel::InspectorPanel(render::Context &context, ViewportPanel &viewport)
    : IEditorPanel("Inspector")
    , mContext(context)
    , mViewport(viewport)
{ }
