#include "stdafx.hpp"

#include "editor/panels/inspector.hpp"

#include "render/render.hpp"

using namespace sm;
using namespace sm::ed;
using namespace sm::math;

template<typename... T>
struct overloaded : T... {
    using T::operator()...;
};

void InspectorPanel::inspect(world::IndexOf<world::Light> index) {
    auto& light = mContext.mWorld.get(index);
    ImGui::InputText("Name", &light.name);

    ImGui::SeparatorText("Properties");

    std::visit(overloaded {
        [&](world::PointLight& it) {
            ImGui::ColorEdit3("Colour", it.colour.data());
            ImGui::DragFloat("Intensity", &it.intensity, 0.1f, 0.0f, 100.0f);
        },
        [&](world::DirectionalLight& it) {
            ImGui::ColorEdit3("Colour", it.colour.data());
            ImGui::DragFloat("Intensity", &it.intensity, 0.1f, 0.0f, 100.0f);
        },
        [&](world::SpotLight& it) {
            ImGui::ColorEdit3("Colour", it.colour.data());
            ImGui::DragFloat("Intensity", &it.intensity, 0.1f, 0.0f, 100.0f);
        },
    }, light.light);
}

void InspectorPanel::inspect(world::IndexOf<world::Image> index) {
    auto& image = mContext.mWorld.get(index);
    ImGui::InputText("Name", &image.name);

    ImGui::SeparatorText("Properties");
    ImGui::Text("Size: %ux%u", image.size.width, image.size.height);
    ImGui::Text("Format: %u", image.format);
    ImGui::Text("Mip Levels: %u", image.mips);

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float aspect = (float)image.size.width / (float)image.size.height;
    float width = avail.x;
    float height = width / aspect;

    auto& texture = mContext.mImages[index];
    auto srv = mContext.mSrvPool.gpu_handle(texture.srv);
    ImGui::Image((ImTextureID)srv.ptr, ImVec2(width, height));
}

void InspectorPanel::inspect(world::IndexOf<world::Material> index) {
    auto& material = mContext.mWorld.get(index);
    ImGui::InputText("Name", &material.name);

    ImGui::SeparatorText("Properties");
    ImGui::ColorEdit4("Albedo", material.albedo.data());

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("IMAGE")) {
            IM_ASSERT(payload->DataSize == sizeof(world::IndexOf<world::Image>));
            auto texture_index = *(world::IndexOf<world::Image>*)payload->Data;
            material.albedo_texture.image = texture_index;
        }
        ImGui::EndDragDropTarget();
    }
}

void InspectorPanel::inspect(world::IndexOf<world::Model> index) {
    auto& model = mContext.mWorld.get(index);
    ImGui::InputText("Name", &model.name);
}

void InspectorPanel::inspect(world::IndexOf<world::Node> index) {
    auto& node = mContext.mWorld.get(index);
    ImGui::InputText("Name", &node.name);

    ImGui::SeparatorText("Children");
    for (auto child : node.children) {
        ImGui::Text("Child %u", child.get());
    }

    ImGui::SeparatorText("Models");
    for (auto model : node.models) {
        ImGui::Text("Model %u", model.get());
    }
}

void InspectorPanel::draw_content() {
    if (mContext.selected.has_value()) {
        std::visit([&](auto index) {
            inspect(index);
        }, mContext.selected.value());
    }
}

void InspectorPanel::draw_window() {
    if (!mOpen) return;

    if (ImGui::Begin("Inspector", &mOpen)) {
        draw_content();
    }
    ImGui::End();
}

InspectorPanel::InspectorPanel(ed::EditorContext &context)
    : mContext(context)
{ }
