#include "stdafx.hpp"

#include "editor/panels/panel.hpp"
#include "editor/panels/assets.hpp"

#include "render/render.hpp"

using namespace sm;
using namespace sm::ed;

using ReflectImageType = ctu::TypeInfo<ImageFormat>;

static const char *get_index_name(world::IndexType type) {
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
static void dragdop_source(world::IndexOf<T> index, const char *name, ImGuiDragDropFlags flags = 0) {
    if (ImGui::BeginDragDropSource(flags)) {
        // typed payload
        ImGui::SetDragDropPayload(name, &index, sizeof(index));

        // generic payload
        world::AnyIndex any { index };
        ImGui::SetDragDropPayload(kIndexPayload, &any, sizeof(world::AnyIndex));
        ImGui::EndDragDropSource();
    }
}

void AssetBrowserPanel::draw_lights() {
    auto& lights = mContext.mWorld.lights;

    draw_grid(lights.size(), [&](size_t i) {
        const auto& light = lights[i];
        const auto& name = light.name;
        auto idx = world::IndexOf<world::Light>(i);

        if (ImGui::Button(name.c_str(), ImVec2(mThumbnailSize, mThumbnailSize))) {
            mContext.selected = idx;
        }

        dragdop_source(idx, "LIGHT");

        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + mThumbnailSize);
        ImGui::TextWrapped("Light %s", name.c_str());
        ImGui::PopTextWrapPos();
    });
}

void AssetBrowserPanel::draw_models() {
    auto& models = mContext.mWorld.models;

    draw_grid(models.size(), [&](size_t i) {
        const auto& model = models[i];
        const auto& name = model.name;
        auto idx = world::IndexOf<world::Model>(i);

        if (ImGui::Button(name.c_str(), ImVec2(mThumbnailSize, mThumbnailSize))) {
            mContext.selected = idx;
        }

        dragdop_source(idx, "MODEL");

        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + mThumbnailSize);
        ImGui::TextWrapped("Model %s", name.c_str());
        ImGui::PopTextWrapPos();
    });
}

void AssetBrowserPanel::draw_images() {
    auto& images = mContext.mWorld.images;

    draw_grid(images.size(), [&](size_t i) {
        const auto& image = images[i];
        const auto& name = image.name;
        auto index = world::IndexOf<world::Image>(i);
        const auto& handle = mContext.mImages[index];
        auto gpu = mContext.mSrvPool.gpu_handle(handle.srv);

        if (ImGui::ImageButton(name.c_str(), (ImTextureID)gpu.ptr, ImVec2(mThumbnailSize, mThumbnailSize))) {
            mContext.selected = index;
        }

        dragdop_source(index, "IMAGE", ImGuiDragDropFlags_SourceAllowNullID);

        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + mThumbnailSize);
        ImGui::TextWrapped("Image %s", name.c_str());
        ImGui::PopTextWrapPos();
    });
}

void AssetBrowserPanel::draw_materials() {
    auto& materials = mContext.mWorld.materials;

    draw_grid(materials.size(), [&](size_t i) {
        const auto& material = materials[i];
        const auto& name = material.name;
        auto idx = world::IndexOf<world::Material>(i);

        if (ImGui::Button(name.c_str(), ImVec2(mThumbnailSize, mThumbnailSize))) {
            mContext.selected = idx;
        }

        dragdop_source(idx, "MATERIAL");

        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + mThumbnailSize);
        ImGui::TextWrapped("Material %s", name.c_str());
        ImGui::PopTextWrapPos();
    });
}

void AssetBrowserPanel::draw_content() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Settings")) {
            ImGui::SliderFloat("Thumbnail Size", &mThumbnailSize, 32.0f, 256.0f);
            ImGui::SliderFloat("Thumbnail Padding", &mThumbnailPadding, 0.0f, 32.0f);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    {
        ImGui::BeginChild("AssetList", ImVec2(150, 0), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX);
        for (int i = world::eNone + 1; i < world::eCount; i++)
        {
            if (ImGui::Selectable(get_index_name((world::IndexType)i), mActiveTab == i))
                mActiveTab = i;
        }
        ImGui::EndChild();
    }
    ImGui::SameLine();

    {
        ImGui::BeginGroup();
        ImGui::BeginChild("AssetView", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));

        switch (mActiveTab) {
        case world::eScene: break;
        case world::eNode: break;
        case world::eCamera: break;
        case world::eModel: draw_models(); break;
        case world::eFile: break;
        case world::eLight: draw_lights(); break;
        case world::eBuffer: break;
        case world::eMaterial: draw_materials(); break;
        case world::eImage: draw_images(); break;
        default: break;
        }

        ImGui::EndChild();
        ImGui::EndGroup();
    }
}

AssetBrowserPanel::AssetBrowserPanel(ed::EditorContext& context)
    : mContext(context)
{ }

void AssetBrowserPanel::draw_window() {
    if (!mOpen) return;

    if (ImGui::Begin("Asset Browser", &mOpen, ImGuiWindowFlags_MenuBar)) {
        draw_content();
    }
    ImGui::End();
}
