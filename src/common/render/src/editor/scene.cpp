#include "render/editor/scene.hpp"

#include "render/render.hpp"

using namespace sm;
using namespace sm::editor;

static constexpr ImGuiTableFlags kTableFlags
    = ImGuiTableFlags_BordersV
    | ImGuiTableFlags_BordersOuterH
    | ImGuiTableFlags_NoHostExtendX
    | ImGuiTableFlags_Resizable
    | ImGuiTableFlags_RowBg
    | ImGuiTableFlags_NoBordersInBody;

const ImGuiTreeNodeFlags kGroupNodeFlags
    = ImGuiTreeNodeFlags_SpanAllColumns
    | ImGuiTreeNodeFlags_OpenOnArrow
    | ImGuiTreeNodeFlags_OpenOnDoubleClick
    | ImGuiTreeNodeFlags_AllowOverlap;

const ImGuiTreeNodeFlags kLeafNodeFlags
    = kGroupNodeFlags
    | ImGuiTreeNodeFlags_Leaf
    | ImGuiTreeNodeFlags_Bullet
    | ImGuiTreeNodeFlags_NoTreePushOnOpen;

static intptr_t unique_id(ItemIndex index) {
    return (intptr_t)index.type.as_integral() << 16 | index.index;
}

static sm::StringView get_item_name(const world::WorldInfo& info, ItemIndex index) {
    switch (index.type) {
    case ItemType::eNode: return info.nodes[index.index].name;
    case ItemType::eMesh: return info.objects[index.index].name;
    default: return "Unknown";
    }
}

bool ScenePanel::begin_tree_node(ItemIndex index, ImGuiTreeNodeFlags flags) {
    auto name = get_item_name(mContext.mWorld.info, index);
    if (index == mSelected) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    const bool open = ImGui::TreeNodeEx((void*)unique_id(index), flags, "%s", name.data());
    if (ImGui::IsItemClicked()) {
        mSelected = index;
        mViewport.select(index);
    }

    if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("ItemIndex", &index, sizeof(ItemIndex));
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("ItemIndex")) {
            ItemIndex data = *(const ItemIndex*)payload->Data;
            if (data != index) {
                //reparent_node(data, index);
                // TODO: reparent node
            }
        }
        ImGui::EndDragDropTarget();
    }

    return open;
}

void ScenePanel::draw_node(uint16 index) {
    auto& node = mContext.mWorld.info.nodes[index];
    ImGui::PushID((void*)&node);
    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    if (node.children.empty()) {
        draw_leaf(index);
    } else {
        draw_group(index);
    }

    ImGui::PopID();
}

void ScenePanel::draw_leaf(uint16 index) {
    begin_tree_node({ItemType::eNode, index}, kLeafNodeFlags);
}

void ScenePanel::draw_group(uint16 index) {
    auto& node = mContext.mWorld.info.nodes[index];
    bool is_open = begin_tree_node({ItemType::eNode, index}, kGroupNodeFlags);

    if (is_open) {
        for (uint16 child : node.children) {
            draw_node(child);
        }

        ImGui::TreePop();
    }
}

void ScenePanel::draw_content() {
    if (ImGui::BeginTable("Scene Tree", 3, kTableFlags)) {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Visible");
        ImGui::TableSetupColumn("Type");
        ImGui::TableHeadersRow();

        draw_node(mContext.mWorld.info.root_node);

        ImGui::EndTable();
    }
}

ScenePanel::ScenePanel(render::Context& context, ViewportPanel& viewport)
    : IEditorPanel("Scene Tree")
    , mContext(context)
    , mViewport(viewport)
{ }
