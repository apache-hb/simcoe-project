#include "render/editor/inspector.hpp"

#include "render/editor/viewport.hpp"

#include "render/render.hpp"

using namespace sm;
using namespace sm::editor;
using namespace sm::math;

void InspectorPanel::draw_mesh(ItemIndex index) {
    SM_ASSERTF(index.type == ItemType::eMesh, "Invalid item type {}", index.type);

    auto& mesh = mContext.mWorld.info.objects[index.index];
    ImGui::Text("Mesh %s selected", mesh.name.c_str());
}

void InspectorPanel::draw_node(ItemIndex index) {
    SM_ASSERTF(index.type == ItemType::eNode, "Invalid item type {}", index.type);

    auto& node = mContext.mWorld.info.nodes[index.index];
    ImGui::Text("Node %s selected", node.name.c_str());

    ImGui::Text("%zu children, %zu objects", node.children.size(), node.objects.size());

    ImGui::SeparatorText("Transform");
    edit_transform(node.transform);

    ImGui::SeparatorText("Gizmo");
    mViewport.gizmo_settings_panel();
}

void InspectorPanel::draw_camera(ItemIndex index) {
    SM_ASSERTF(index.type == ItemType::eCamera, "Invalid item type {}", index.type);

    auto& camera = mContext.mWorld.info.cameras[index.index];
    ImGui::Text("Camera %s selected", camera.name.c_str());
}

void InspectorPanel::draw_image(ItemIndex index) {
    SM_ASSERTF(index.type == ItemType::eImage, "Invalid item type {}", index.type);

    // TODO: really need a better way to handle texture indices
    auto& image = mContext.mWorld.info.images[index.index];
    auto& texture = mContext.mTextures[index.index];
    ImGui::Text("Image %s selected", image.name.c_str());

    ImGui::Text("%u x %u (%u mips)", texture.size.width, texture.size.height, texture.mips);
    using Reflect = ctu::TypeInfo<ImageFormat>;
    ImGui::Text("Format: %s", Reflect::to_string(texture.format).c_str());
    sm::Memory usage = texture.data.size();
    ImGui::Text("Size: %s", usage.to_string().c_str());

    auto srv = mContext.mSrvPool.gpu_handle(texture.srv);

    ImGui::Image((ImTextureID)srv.ptr, ImVec2(128, 128));
}

void InspectorPanel::draw_material(ItemIndex index) {
    SM_ASSERTF(index.type == ItemType::eMaterial, "Invalid item type {}", index.type);

    auto& material = mContext.mWorld.info.materials[index.index];
    ImGui::Text("Material %s selected", material.name.c_str());
}

void InspectorPanel::draw_content() {
    auto selected = mViewport.get_selected();
    switch (selected.type) {
    case ItemType::eNone:
        ImGui::Text("Nothing selected");
        return;
    case ItemType::eNode:
        draw_node(selected);
        break;
    case ItemType::eMesh:
        draw_mesh(selected);
        break;
    case ItemType::eCamera:
        draw_camera(selected);
        break;
    case ItemType::eImage:
        draw_image(selected);
        break;
    case ItemType::eMaterial:
        draw_material(selected);
        break;
    }
}

InspectorPanel::InspectorPanel(render::Context &context, ViewportPanel &viewport)
    : IEditorPanel("Inspector")
    , mContext(context)
    , mViewport(viewport)
{ }
