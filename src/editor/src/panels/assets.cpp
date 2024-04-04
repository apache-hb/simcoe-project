#include "editor/panel.hpp"
#include "stdafx.hpp"

#include "editor/assets.hpp"

#include "render/render.hpp"

using namespace sm;
using namespace sm::ed;

using ReflectImageType = ctu::TypeInfo<ImageFormat>;

namespace MyGui {
    static void TextCutoff(const char *text, float width) {
        ImVec2 cursor = ImGui::GetCursorPos();
        ImGui::PushTextWrapPos(cursor.x + width);
        ImGui::Text("%s", text);
        ImGui::PopTextWrapPos();
    }
}

void AssetBrowserPanel::draw_models() {
    auto& models = mContext.mWorld.info.objects;

    ImVec2 avail = ImGui::GetContentRegionAvail();
    uint columns = (uint)(avail.x / (mThumbnailSize + mThumbnailPadding));
    if (columns < 1) columns = 1;

    for (size_t i = 0; i < models.size(); i++) {
        const auto& model = models[i];
        const auto& mesh = mContext.mMeshes[i];

        ImGui::PushID((int)i);
        ImGui::BeginGroup();
        using Reflect = ctu::TypeInfo<world::ObjectType>;
        if (ImGui::Button(model.name.c_str(), ImVec2(mThumbnailSize, mThumbnailSize))) {
            mContext.selected = ItemIndex{ ItemType::eMesh, (uint16)i };
        }

        if (ImGui::BeginDragDropSource()) {
            ItemIndex index{ ItemType::eMesh, (uint16)i };
            ImGui::SetDragDropPayload(kMeshPayload, &index, sizeof(ItemIndex));
            ImGui::EndDragDropSource();
        }
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + mThumbnailSize);
        ImGui::TextWrapped("Model %s (%s)", model.name.c_str(), Reflect::to_string(mesh.mInfo.type).c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndGroup();
        ImGui::PopID();

        if ((i + 1) % columns != 0) {
            ImGui::SameLine();
        }
    }
}

void AssetBrowserPanel::draw_images() {
    auto& textures = mContext.mTextures;

    ImVec2 avail = ImGui::GetContentRegionAvail();
    uint columns = (uint)(avail.x / (mThumbnailSize + mThumbnailPadding));
    if (columns < 1) columns = 1;

    for (size_t i = 0; i < textures.size(); i++) {
        const auto& texture = textures[i];
        const auto& path = texture.path;

        const auto& name = texture.name;
        auto gpu = mContext.mSrvPool.gpu_handle(texture.srv);
        ImGui::PushID((int)i);
        ImGui::BeginGroup();
        if (ImGui::ImageButton(name.c_str(), (ImTextureID)gpu.ptr, math::float2(mThumbnailSize))) {
            mContext.selected = ItemIndex{ ItemType::eImage, (uint16)i };
        }
        if (ImGui::BeginItemTooltip()) {
            ImGui::Text("%s (%s)", path.string().c_str(), ReflectImageType::to_string(texture.format).c_str());
            auto [w, h] = texture.size;
            ImGui::Text("%u x %u (%u mips)", w, h, texture.mips);
            ImGui::EndTooltip();
        }
        MyGui::TextCutoff(name.c_str(), mThumbnailSize);
        ImGui::EndGroup();
        ImGui::PopID();

        if ((i + 1) % columns != 0) {
            ImGui::SameLine();
        }
    }
}

void AssetBrowserPanel::draw_materials() {

}

void AssetBrowserPanel::draw_content() {
    if (ImGui::Button("Import")) {
        mFileBrowser.Open();
    }

    ImGui::SameLine();

    if (ImGui::Button("Settings")) {
        ImGui::OpenPopup("Settings");
    }

    if (ImGui::BeginPopup("Settings")) {
        ImGui::SliderFloat("Thumbnail Size", &mThumbnailSize, 32.0f, 256.0f);
        ImGui::SliderFloat("Thumbnail Padding", &mThumbnailPadding, 0.0f, 32.0f);
        ImGui::EndPopup();
    }

    if (ImGui::BeginTabBar("Assets", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton)) {
        if (ImGui::BeginTabItem("Models")) {
            draw_models();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Images")) {
            draw_images();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Materials")) {
            draw_materials();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    mFileBrowser.Display();

    if (mFileBrowser.HasSelected()) {
        auto selected = mFileBrowser.GetMultiSelected();
        for (const auto& file : selected) {
            auto ext = file.extension();
            if (ext == ".png" || ext == ".jpg" || ext == ".bmp" || ext == ".dds")
                mContext.load_texture(file);
            else if (ext == ".glb" || ext == ".gltf")
                mContext.load_gltf(file);
        }
        mFileBrowser.ClearSelected();
    }
}

AssetBrowserPanel::AssetBrowserPanel(ed::EditorContext& context)
    : mContext(context)
{
    mFileBrowser.SetTitle("Import Assets");
}

void AssetBrowserPanel::draw_window() {
    if (!mOpen) return;

    if (ImGui::Begin("Asset Browser", &mOpen)) {
        draw_content();
    }
    ImGui::End();
}
