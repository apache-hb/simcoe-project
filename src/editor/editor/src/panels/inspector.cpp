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

template<world::IsWorldObject T>
static void drawItemInspector(Inspector& self, world::IndexOf<T> index) {
    ImGui::TextUnformatted("Unimplemented :(");
}

static void drawItemInspector(Inspector& self, world::IndexOf<world::Light> index) {
    auto& light = self.ctx.mWorld.get(index);
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

static void drawItemInspector(Inspector& self, world::IndexOf<world::Image> index) {
    auto& image = self.ctx.mWorld.get(index);
    ImGui::InputText("Name", &image.name);

    ImGui::SeparatorText("Properties");
    ImGui::Text("Size: %ux%u", image.size.width, image.size.height);
    ImGui::Text("Format: %u", image.format);
    ImGui::Text("Mip Levels: %u", image.mips);

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float aspect = (float)image.size.width / (float)image.size.height;
    float width = avail.x;
    float height = width / aspect;

    auto& texture = self.ctx.mImages[index];
    auto srv = self.ctx.mSrvPool.gpu_handle(texture.srv);
    ImGui::Image((ImTextureID)srv.ptr, ImVec2(width, height));
}

static void drawItemInspector(Inspector& self, world::IndexOf<world::Material> index) {
    auto& material = self.ctx.mWorld.get(index);
    ImGui::InputText("Name", &material.name);

    ImGui::SeparatorText("Properties");
    ImGui::ColorEdit3("Albedo", material.albedo.data());

    if (material.albedo_texture.image != world::kInvalidIndex) {
        auto& texture = self.ctx.mImages[material.albedo_texture.image];
        auto srv = self.ctx.mSrvPool.gpu_handle(texture.srv);
        ImGui::Image((ImTextureID)srv.ptr, ImVec2(64, 64));
    }

    // draw an image selector
    if (ImGui::BeginCombo("Albedo Texture", "Select Texture")) {
        for (auto& [index, image] : self.ctx.mImages) {
            const auto& info = self.ctx.mWorld.get(index);
            if (ImGui::Selectable(info.name.c_str())) {
                material.albedo_texture.image = index;
            }
        }
        ImGui::EndCombo();
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("IMAGE")) {
            IM_ASSERT(payload->DataSize == sizeof(world::IndexOf<world::Image>));
            auto texture_index = *(world::IndexOf<world::Image>*)payload->Data;
            material.albedo_texture.image = texture_index;
        }
        ImGui::EndDragDropTarget();
    }
}

static const char *dxgiFormatString(DXGI_FORMAT it) {
    switch (it) {
    case DXGI_FORMAT_R16_UINT: return "R16_UINT";
    case DXGI_FORMAT_R32_UINT: return "R32_UINT";

    default: return "Unknown";
    }
}

static void drawFeature(const char *name, bool enabled) {
    if (enabled) {
        ImGui::Text("%s: Enabled", name);
    } else {
        ImGui::TextDisabled("%s: Disabled", name);
    }
}

static void drawItemInspector(Inspector& self, world::IndexOf<world::Model> index) {
    auto& model = self.ctx.mWorld.get(index);
    ImGui::InputText("Name", &model.name);

    if (const world::Object *object = std::get_if<world::Object>(&model.mesh)) {
        ImGui::Text("Vertex Count: %u", object->getVertexCount());
        ImGui::Text("Index Count: %u", object->getIndexCount());
    } else {
        ImGui::Text("Primitive Mesh");
    }

    DXGI_FORMAT indexFormat = model.getIndexBufferFormat();
    ImGui::Text("Index Format: %s", dxgiFormatString(indexFormat));

    world::VertexFlags flags = model.getVertexBufferFlags();
    if (ImGui::TreeNodeEx("Flags")) {
        drawFeature("Positions", flags.test(world::VertexFlags::ePositions));
        drawFeature("Vertex normals", flags.test(world::VertexFlags::eNormals));
        drawFeature("Texture coordinates", flags.test(world::VertexFlags::eTexCoords));
        drawFeature("Tangents", flags.test(world::VertexFlags::eTangents));

        ImGui::TreePop();
    }

    ImGui::SeparatorText("Properties");
    if (model.material != world::kInvalidIndex) {
        auto& material = self.ctx.mWorld.get(model.material);
        ImGui::Text("Material: %s", material.name.c_str());
        if (material.albedo_texture.image != world::kInvalidIndex) {
            auto& texture = self.ctx.mImages[material.albedo_texture.image];
            auto srv = self.ctx.mSrvPool.gpu_handle(texture.srv);
            ImGui::Image((ImTextureID)srv.ptr, ImVec2(64, 64));
        } else {
            ImGui::TextDisabled("No Albedo Texture");
        }
    } else {
        ImGui::TextDisabled("No Material");
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MATERIAL")) {
            IM_ASSERT(payload->DataSize == sizeof(world::IndexOf<world::Material>));
            auto material_index = *(world::IndexOf<world::Material>*)payload->Data;
            model.material = material_index;
        }
        ImGui::EndDragDropTarget();
    }
}

static void drawItemInspector(Inspector& self, world::IndexOf<world::Node> index) {
    auto& node = self.ctx.mWorld.get(index);
    ImGui::InputText("Name", &node.name);

    auto& [t, r, s] = node.transform;

    ImGui::DragFloat3("Position", t.data(), 0.1f);

    auto e = r.asEulerAngle().get_degrees();
    ImGui::Text("Rotation: %f.%f.%f", e.roll, e.pitch, e.yaw);
    ImGui::Text("Rotation: %f.%f.%f (%f)", r.v.x, r.v.y, r.v.z, r.w);
    // MyGui::DragAngle3("Rotation", &r, 1._deg, 0._deg, 360._deg);
    ImGui::DragFloat3("Scale", s.data(), 0.1f);

    ImGui::SeparatorText("Children");
    for (auto child : node.children) {
        ImGui::Text("Child %u", child.get());
    }

    ImGui::SeparatorText("Models");
    for (auto model : node.models) {
        ImGui::Text("Model %u", model.get());
    }
}

static MemoryEditor& getMemoryEditor(Inspector& self, world::IndexOf<world::Buffer> index) {
    auto it = self.memoryEditors.find(index);
    if (it == self.memoryEditors.end()) {
        auto [it, _] = self.memoryEditors.emplace(index, MemoryEditor());
        MemoryEditor& editor = it->second;
        editor.Open = false;
        return editor;
    }

    return it->second;
}

static void drawItemInspector(Inspector& self, world::IndexOf<world::Buffer> index) {
    auto& buffer = self.ctx.mWorld.get(index);
    ImGui::InputText("Name", &buffer.name);
    ImGui::Text("Size: %zu", buffer.data.size());

    if (ImGui::Button("Open Memory Editor")) {
        auto& editor = getMemoryEditor(self, index);
        editor.Open = true;
    }

    ImGui::SeparatorText("Data");

    MemoryEditor& editor = getMemoryEditor(self, index);
    editor.DrawContents(buffer.data.data(), buffer.data.size());
}

static void drawInspectorContent(Inspector& self) {
    if (self.ctx.selected.has_value()) {
        std::visit([&](auto index) {
            drawItemInspector(self, index);
        }, self.ctx.selected.value());
    }
}

void Inspector::draw_window() {
    for (auto& [index, editor] : memoryEditors) {
        if (!editor.Open) continue;

        world::Buffer& info = ctx.mWorld.get(index);
        editor.DrawWindow(info.name.c_str(), info.data.data(), info.data.size());
    }

    if (!isOpen) return;

    if (ImGui::Begin("Inspector", &isOpen)) {
        drawInspectorContent(*this);
    }
    ImGui::End();
}

Inspector::Inspector(ed::EditorContext &context)
    : ctx(context)
{ }
