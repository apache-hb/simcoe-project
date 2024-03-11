#include "stdafx.hpp"

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

static auto gSink = logs::get_sink(logs::Category::eDebug);

static intptr_t unique_id(ItemIndex index) {
    return (intptr_t)index.type.as_integral() << 16 | index.index;
}

static sm::StringView get_item_name(const world::WorldInfo& info, ItemIndex index) {
    switch (index.type) {
    case ItemType::eNode: return info.nodes[index.index].name;
    case ItemType::eMesh: return info.objects[index.index].name;
    case ItemType::eCamera: return info.cameras[index.index].name;
    case ItemType::eImage: return info.images[index.index].name;
    case ItemType::eMaterial: return info.materials[index.index].name;
    default: return "Unknown";
    }
}

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
    auto& world = mContext.mWorld.info;
    auto name = get_item_name(world, self);
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
            world.reparent_node(node.index, self.index);
        }

        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(kMeshPayload)) {
            ItemIndex mesh = *(const ItemIndex*)payload->Data;
            SM_ASSERTF(mesh.type == ItemType::eMesh, "Invalid payload type {}", mesh.type);
            world.add_node_object(self.index, mesh.index);
        }
        ImGui::EndDragDropTarget();
    }

    uint16 index = self.index;

    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::Button("Add Child")) {
            mNodeInfo.parent = index;
            uint16 id = world.add_node(mNodeInfo);
            world.reparent_node(id, index);
            ImGui::CloseCurrentPopup();
        }

        if (!world.is_root_node(index)) {
            ImGui::SameLine();
            if (ImGui::Button("Delete")) {
                world.delete_node(index);
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::InputText("Name", &mNodeInfo.name);
        edit_transform(mNodeInfo.transform);

        ImGui::EndPopup();
    }

    return open;
}

void ScenePanel::draw_node(uint16 index) {
    auto& world = mContext.mWorld.info;
    const auto& node = world.nodes[index];
    ImGui::PushID(index);
    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    bool leaf = node.children.empty();
    if (mShowObjects && !node.objects.empty()) {
        leaf = false;
    }

    if (leaf) {
        draw_leaf(index);
    } else {
        draw_group(index);
    }

    ImGui::PopID();
}

void ScenePanel::draw_leaf(uint16 index) {
    begin_tree_item({ItemType::eNode, index}, kLeafNodeFlags);
    ImGui::TableNextColumn();

    ImGui::TableNextColumn();
    ImGui::TextUnformatted("Node");
}

void ScenePanel::draw_group(uint16 index) {
    bool is_open = begin_tree_item({ItemType::eNode, index}, kGroupNodeFlags);

    ImGui::TableNextColumn();

    ImGui::TableNextColumn();
    ImGui::TextUnformatted("Node");

    auto& world = mContext.mWorld.info;
    auto& node = world.nodes[index];

    if (is_open) {
        if (mShowObjects) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImVec4 cyan = {0.5f, 1.f, 1.f, 1.f};
            ImGui::PushStyleColor(ImGuiCol_Text, cyan);
            if (ImGui::TreeNodeEx("Objects", kGroupNodeFlags)) {
                ImGui::PopStyleColor();
                for (uint16 i : node.objects) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    begin_tree_item({ItemType::eMesh, i}, kLeafNodeFlags);

                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    using Reflect = ctu::TypeInfo<world::ObjectType>;
                    ImGui::TextUnformatted(Reflect::to_string(world.objects[i].info.type).c_str());
                }
                ImGui::TreePop();
            } else {
                ImGui::PopStyleColor();
            }
        }

        for (size_t i = 0; i < node.children.size(); ++i) {
            draw_node(node.children[i]);
        }

        ImGui::TreePop();
    }
}

void ScenePanel::draw_cameras() {
    auto& world = mContext.mWorld.info;
    for (size_t i = 0; i < world.cameras.size(); i++) {
        ImGui::PushID(i);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        begin_tree_item({ItemType::eCamera, (uint16)i}, kLeafNodeFlags);
        ImGui::TableNextColumn();

        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Camera");
        ImGui::PopID();
    }
}

void ScenePanel::draw_content() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Show")) {
            ImGui::MenuItem("Objects", nullptr, &mShowObjects);
            ImGui::MenuItem("Cameras", nullptr, &mShowCameras);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    if (ImGui::BeginTable("Scene Tree", 3, kTableFlags)) {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Visible");
        ImGui::TableSetupColumn("Type");
        ImGui::TableHeadersRow();

        draw_node(mContext.mWorld.info.root_node);

        if (mShowCameras) {
            draw_cameras();
        }

        ImGui::EndTable();
    }
}

ScenePanel::ScenePanel(render::Context& context, ViewportPanel& viewport)
    : IEditorPanel("Scene Tree")
    , mContext(context)
    , mViewport(viewport)
{
    mFlags |= ImGuiWindowFlags_MenuBar;
}
