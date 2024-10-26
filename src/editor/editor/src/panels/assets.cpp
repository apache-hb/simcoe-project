#include "stdafx.hpp"

#include "editor/panels/panel.hpp"
#include "editor/panels/assets.hpp"

#include "render/render.hpp"

using namespace sm;
using namespace sm::ed;

static const char *getIndexName(world::IndexType type) {
    switch (type) {
    case world::eNone: return "None";
    case world::eScene: return "Scene";
    case world::eNode: return "Node";
    case world::eCamera: return "Camera";
    case world::eModel: return "Model";
    case world::eFile: return "File";
    case world::eLight: return "Light";
    case world::eBuffer: return "Buffer";
    case world::eMaterial: return "Material";
    case world::eImage: return "Image";
    default: return "Unknown";
    }
}

template<world::IsWorldObject T>
static void makeDragDropSource(world::IndexOf<T> index, const char *name, ImGuiDragDropFlags flags = 0) {
    if (ImGui::BeginDragDropSource(flags)) {
        // typed payload
        ImGui::SetDragDropPayload(name, &index, sizeof(index));

        // generic payload
        world::AnyIndex any { index };
        ImGui::SetDragDropPayload(kIndexPayload, &any, sizeof(world::AnyIndex));
        ImGui::EndDragDropSource();
    }
}

static void drawAssetGrid(AssetBrowser& self, size_t count, auto&& fn) {
    ImVec2 avail = ImGui::GetContentRegionAvail();
    uint columns = (uint)(avail.x / (self.thumbnailSize + self.thumbnailPadding));
    if (columns < 1) columns = 1;

    for (size_t i = 0; i < count; i++) {
        ImGui::PushID((int)i);
        ImGui::BeginGroup();
        fn(i);
        ImGui::EndGroup();
        ImGui::PopID();

        if ((i + 1) % columns != 0) {
            ImGui::SameLine();
        }
    }
}

template<world::IsWorldObject T>
static void drawSimpleGrid(AssetBrowser& self, const char *kind, const char *tag, sm::Span<const T> elements) {
    drawAssetGrid(self, elements.size(), [&](size_t i) {
        const auto& element = elements[i];
        const auto& name = element.name;
        auto idx = world::IndexOf<T>(i);

        if (ImGui::Button(name.c_str(), ImVec2(self.thumbnailSize, self.thumbnailSize))) {
            self.ctx.selected = idx;
        }

        makeDragDropSource(idx, tag);

        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + self.thumbnailSize);
        ImGui::TextWrapped("%s %s", kind, name.c_str());
        ImGui::PopTextWrapPos();
    });
}

static void drawLightAssets(AssetBrowser& self) {
    auto& lights = self.ctx.mWorld.mLights;

    drawAssetGrid(self, lights.size(), [&](size_t i) {
        const auto& light = lights[i];
        const auto& name = light.name;
        auto idx = world::IndexOf<world::Light>(i);

        if (ImGui::Button(name.c_str(), ImVec2(self.thumbnailSize, self.thumbnailSize))) {
            self.ctx.selected = idx;
        }

        makeDragDropSource(idx, "LIGHT");

        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + self.thumbnailSize);
        ImGui::TextWrapped("Light %s", name.c_str());
        ImGui::PopTextWrapPos();
    });
}

static void drawModelAssets(AssetBrowser& self) {
    auto& models = self.ctx.mWorld.all<world::Model>();

    drawSimpleGrid<world::Model>(self, "Model", "MODEL", models);
}

static void drawImageAssets(AssetBrowser& self) {
    auto& images = self.ctx.mWorld.all<world::Image>();

    drawAssetGrid(self, images.size(), [&](size_t i) {
        const auto& image = images[i];
        const auto& name = image.name;
        auto index = world::IndexOf<world::Image>(i);
        const auto& handle = self.ctx.mImages[index];
        auto gpu = self.ctx.mSrvPool.gpu_handle(handle.srv);

        if (ImGui::ImageButton(name.c_str(), (ImTextureID)gpu.ptr, ImVec2(self.thumbnailSize, self.thumbnailSize))) {
            self.ctx.selected = index;
        }

        makeDragDropSource(index, "IMAGE", ImGuiDragDropFlags_SourceAllowNullID);

        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + self.thumbnailSize);
        ImGui::TextWrapped("Image %s", name.c_str());
        ImGui::PopTextWrapPos();
    });
}

static void drawMaterialAssets(AssetBrowser& self) {
    auto& materials = self.ctx.mWorld.mMaterials;
    drawSimpleGrid<world::Material>(self, "Material", "MATERIAL", materials);
}

static void drawFileAssets(AssetBrowser& self) {
    auto& files = self.ctx.mWorld.mFiles;

    drawAssetGrid(self, files.size(), [&](size_t i) {
        const auto& file = files[i];
        const auto& name = file.path;
        auto idx = world::IndexOf<world::File>(i);

        if (ImGui::Button(name.c_str(), ImVec2(self.thumbnailSize, self.thumbnailSize))) {
            self.ctx.selected = idx;
        }

        makeDragDropSource(idx, "FILE");

        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + self.thumbnailSize);
        ImGui::TextWrapped("File %s", name.c_str());
        ImGui::PopTextWrapPos();
    });
}

static void drawBufferAssets(AssetBrowser& self) {
    auto& buffers = self.ctx.mWorld.mBuffers;

    drawSimpleGrid<world::Buffer>(self, "Buffer", "BUFFER", buffers);
}

static void drawNodeAssets(AssetBrowser& self) {
    auto& nodes = self.ctx.mWorld.mNodes;

    drawSimpleGrid<world::Node>(self, "Node", "NODE", nodes);
}

static void drawCameraAssets(AssetBrowser& self) {
    auto& cameras = self.ctx.mWorld.mCameras;

    drawSimpleGrid<world::Camera>(self, "Camera", "CAMERA", cameras);
}

static void drawAssetBrowser(AssetBrowser& self) {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Settings")) {
            ImGui::SliderFloat("Thumbnail Size", &self.thumbnailSize, 32.0f, 256.0f);
            ImGui::SliderFloat("Thumbnail Padding", &self.thumbnailPadding, 0.0f, 32.0f);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    {
        ImGui::BeginChild("AssetList", ImVec2(150, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
        for (int i = world::eNone + 1; i < world::eCount; i++)
        {
            if (ImGui::Selectable(getIndexName((world::IndexType)i), self.activeTab == i))
                self.activeTab = i;
        }
        ImGui::EndChild();
    }
    ImGui::SameLine();

    {
        ImGui::BeginGroup();
        ImGui::BeginChild("AssetView", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));

        switch (self.activeTab) {
        case world::eScene: break;
        case world::eNode: drawNodeAssets(self); break;
        case world::eCamera: drawCameraAssets(self); break;
        case world::eModel: drawModelAssets(self); break;
        case world::eFile: drawFileAssets(self); break;
        case world::eLight: drawLightAssets(self); break;
        case world::eBuffer: drawBufferAssets(self); break;
        case world::eMaterial: drawMaterialAssets(self); break;
        case world::eImage: drawImageAssets(self); break;
        default: break;
        }

        ImGui::EndChild();
        ImGui::EndGroup();
    }
}

AssetBrowser::AssetBrowser(ed::EditorContext& context)
    : ctx(context)
{ }

void AssetBrowser::draw_window() {
    if (!isOpen) return;

    if (ImGui::Begin("Asset Browser", &isOpen, ImGuiWindowFlags_MenuBar)) {
        drawAssetBrowser(*this);
    }
    ImGui::End();
}
