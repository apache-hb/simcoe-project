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

static constexpr const char *kNodePayload = "NodeIndex";
static constexpr const char *kMeshPayload = "MeshIndex";

static const char *get_payload_type(ItemType type) {
    switch (type) {
    case ItemType::eNode:
        return kNodePayload;
    case ItemType::eMesh:
        return kMeshPayload;

    default:
        SM_NEVER("Unknown item type {}", type);
        return "Unknown";
    }
}

bool ScenePanel::begin_tree_item(ItemIndex self, ImGuiTreeNodeFlags flags) {
    auto name = get_item_name(mContext.mWorld.info, self);
    if (self == mSelected) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    const bool open = ImGui::TreeNodeEx((void*)unique_id(self), flags, "%s", name.data());
    if (ImGui::IsItemClicked()) {
        mSelected = self;
        mViewport.select(self);
    }

    if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload(get_payload_type(self.type), &self, sizeof(ItemIndex));
        ImGui::EndDragDropSource();
    }

    if (self.type == ItemType::eMesh) return open;

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(kNodePayload)) {
            ItemIndex node = *(const ItemIndex*)payload->Data;
            SM_ASSERTF(node.type == ItemType::eNode, "Invalid payload type {}", node.type);
            mContext.mWorld.info.reparent_node(node.index, self.index);
        }
        ImGui::EndDragDropTarget();

        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(kMeshPayload)) {
            ItemIndex mesh = *(const ItemIndex*)payload->Data;
            SM_ASSERTF(mesh.type == ItemType::eMesh, "Invalid payload type {}", mesh.type);
            mContext.mWorld.info.add_object(self.index, mesh.index);
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
    begin_tree_item({ItemType::eNode, index}, kLeafNodeFlags);
}

void ScenePanel::draw_group(uint16 index) {
    auto& node = mContext.mWorld.info.nodes[index];
    bool is_open = begin_tree_item({ItemType::eNode, index}, kGroupNodeFlags);

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
