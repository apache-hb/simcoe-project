#include "render/editor/inspector.hpp"

#include "render/render.hpp"

using namespace sm;
using namespace sm::editor;

void InspectorPanel::draw_content() {
    switch (mSelected.type) {
    case ItemIndex::eNone:
        ImGui::Text("Nothing selected");
        break;
    case ItemIndex::eNode:
        ImGui::Text("Node %s selected", mContext.mWorld.nodes[mSelected.index].name.data());
        break;
    case ItemIndex::eObject:
        ImGui::Text("Object %s selected", mContext.mWorld.objects[mSelected.index].name.data());
        break;
    }
}

InspectorPanel::InspectorPanel(render::Context &context)
    : IEditorPanel("Inspector")
    , mContext(context)
{ }
