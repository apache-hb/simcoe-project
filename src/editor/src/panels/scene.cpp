#include "editor/panel.hpp"
#include "stdafx.hpp"

#include "editor/scene.hpp"

#include "render/render.hpp"

using namespace sm;
using namespace sm::ed;

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

static const char *const kCreatePopup = "New Primitive";

namespace MyGui {
template<ctu::Reflected T>
    requires (ctu::is_enum<T>())
static bool CheckboxFlags(const char *label, T &flags, T flag) {
    unsigned val = flags.as_integral();
    if (ImGui::CheckboxFlags(label, &val, flag.as_integral())) {
        flags = T(val);
        return true;
    }
    return false;
}

template <ctu::Reflected T>
    requires(ctu::is_enum<T>())
static bool EnumCombo(const char *label, typename ctu::TypeInfo<T>::Type &choice) {
    using Reflect = ctu::TypeInfo<T>;
    const auto id = Reflect::to_string(choice);
    if (ImGui::BeginCombo(label, id.c_str())) {
        for (size_t i = 0; i < std::size(Reflect::kCases); i++) {
            const auto &[name, value] = Reflect::kCases[i];
            bool selected = choice == value;
            if (ImGui::Selectable(name.c_str(), selected)) {
                choice = value;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    return true;
}
} // namespace MyGui

static constexpr world::MeshInfo get_default_info(world::ObjectType type) {
    switch (type.as_enum()) {
    case world::ObjectType::eCube: return {.type = type, .cube = {1.f, 1.f, 1.f}};
    case world::ObjectType::eSphere: return {.type = type, .sphere = {1.f, 6, 6}};
    case world::ObjectType::eCylinder: return {.type = type, .cylinder = {1.f, 1.f, 8}};
    case world::ObjectType::ePlane: return {.type = type, .plane = {1.f, 1.f}};
    case world::ObjectType::eWedge: return {.type = type, .wedge = {1.f, 1.f, 1.f}};
    case world::ObjectType::eCapsule: return {.type = type, .capsule = {1.f, 5.f, 12, 4}};
    case world::ObjectType::eGeoSphere: return {.type = type, .geosphere = {1.f, 2}};
    default: return {.type = type};
    }
}

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
    case ItemType::eCamera:
        return kCameraPayload;

    default:
        logs::gDebug.warn("Unknown item type {}", type);
        return "Unknown";
    }
}

bool ScenePanel::begin_tree_item(ItemIndex self, ImGuiTreeNodeFlags flags) {
    auto& world = mContext.mWorld.info;
    auto name = get_item_name(world, self);
    if (self == mContext.selected) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    const bool open = ImGui::TreeNodeEx((void*)unique_id(self), flags, "%s", name.data());
    if (ImGui::IsItemClicked()) {
        mContext.selected = self;
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

void ScenePanel::create_primitive() {
    if (ImGui::BeginPopup(kCreatePopup)) {
        static world::ObjectType type = world::ObjectType::eCube;
        static math::float3 colour = {1.f, 1.f, 1.f};
        MyGui::EnumCombo<world::ObjectType>("Type", type);
        ImGui::InputText("Name", &mMeshName);

        auto &info = mMeshCreateInfo[type];
        auto &world = mContext.mWorld.info;

        switch (type.as_enum()) {
        case world::ObjectType::eCube:
            ImGui::SliderFloat("Width", &info.cube.width, 0.1f, 10.f);
            ImGui::SliderFloat("Height", &info.cube.height, 0.1f, 10.f);
            ImGui::SliderFloat("Depth", &info.cube.depth, 0.1f, 10.f);
            break;
        case world::ObjectType::eSphere:
            ImGui::SliderFloat("Radius", &info.sphere.radius, 0.1f, 10.f);
            ImGui::SliderInt("Slices", &info.sphere.slices, 3, 32);
            ImGui::SliderInt("Stacks", &info.sphere.stacks, 3, 32);
            break;
        case world::ObjectType::eCylinder:
            ImGui::SliderFloat("Radius", &info.cylinder.radius, 0.1f, 10.f);
            ImGui::SliderFloat("Height", &info.cylinder.height, 0.1f, 10.f);
            ImGui::SliderInt("Slices", &info.cylinder.slices, 3, 32);
            break;
        case world::ObjectType::ePlane:
            ImGui::SliderFloat("Width", &info.plane.width, 0.1f, 10.f);
            ImGui::SliderFloat("Depth", &info.plane.depth, 0.1f, 10.f);
            break;
        case world::ObjectType::eWedge:
            ImGui::SliderFloat("Width", &info.wedge.width, 0.1f, 10.f);
            ImGui::SliderFloat("Depth", &info.wedge.depth, 0.1f, 10.f);
            ImGui::SliderFloat("Height", &info.wedge.height, 0.1f, 10.f);
            break;
        case world::ObjectType::eCapsule:
            ImGui::SliderFloat("Radius", &info.capsule.radius, 0.1f, 10.f);
            ImGui::SliderFloat("Height", &info.capsule.height, 0.1f, 10.f);
            ImGui::SliderInt("Slices", &info.capsule.slices, 3, 32);
            ImGui::SliderInt("Rings", &info.capsule.rings, 3, 8);
            break;
        case world::ObjectType::eGeoSphere:
            ImGui::SliderFloat("Radius", &info.geosphere.radius, 0.1f, 10.f);
            ImGui::SliderInt("Subdivisions", &info.geosphere.subdivisions, 1, 8);
            break;
        default: ImGui::Text("Unimplemented primitive type"); break;
        }

        ImGui::ColorEdit3("Colour", colour.data(), ImGuiColorEditFlags_Float);

        if (ImGui::Button("Create")) {
            world::ObjectInfo obj = { .name = mMeshName, .info = info };
            world.add_object(obj);
            mContext.mMeshes.push_back(mContext.create_mesh(info, colour));
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void ScenePanel::draw_content() {
    ImGuiID id = ImGui::GetID(kCreatePopup);
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Show")) {
            ImGui::MenuItem("Objects", nullptr, &mShowObjects);
            ImGui::MenuItem("Cameras", nullptr, &mShowCameras);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Create")) {
            if (ImGui::MenuItem("Primitive")) {
                ImGui::OpenPopup(id);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    create_primitive();

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

ScenePanel::ScenePanel(ed::EditorContext& context)
    : mContext(context)
{
    auto cases = world::ObjectType::cases();
    for (world::ObjectType i : cases) {
        mMeshCreateInfo[i] = get_default_info(i);
    }
}

void ScenePanel::draw_window() {
    if (!mOpen) return;

    if (ImGui::Begin("Scene Tree", &mOpen, ImGuiWindowFlags_MenuBar)) {
        draw_content();
    }
    ImGui::End();
}
