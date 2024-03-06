#include "render/editor/assets.hpp"

#include "render/render.hpp"

using namespace sm;
using namespace sm::editor;

using ReflectImageType = ctu::TypeInfo<ImageFormat>;

namespace MyGui {
    void TextCutoff(const char *text, float width) {
        ImVec2 cursor = ImGui::GetCursorPos();
        ImGui::PushTextWrapPos(cursor.x + width);
        ImGui::Text("%s", text);
        ImGui::PopTextWrapPos();
    }
}

void AssetBrowserPanel::draw_images() {
    auto& textures = mContext.mTextures;

    ImVec2 avail = ImGui::GetContentRegionAvail();
    uint columns = (uint)(avail.x / (mThumbnailSize + mThumbnailPadding));
    if (columns < 1) columns = 1;

    for (size_t i = 0; i < textures.size(); i++) {
        const auto& texture = textures[i];
        const auto& name = texture.name;
        auto gpu = mContext.mSrvPool.gpu_handle(texture.srv);
        ImGui::PushID((int)i);
        ImGui::BeginGroup();
        if (ImGui::ImageButton(name.c_str(), (ImTextureID)gpu.ptr, math::float2(mThumbnailSize))) {

        }
        ImGui::SetItemTooltip("%s", name.c_str());
        MyGui::TextCutoff(name.c_str(), mThumbnailSize);
        ImGui::EndGroup();
        ImGui::PopID();

        if ((i + 1) % columns != 0) {
            ImGui::SameLine();
        }
    }
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
        if (ImGui::BeginTabItem("Images")) {
            draw_images();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    mFileBrowser.Display();

    if (mFileBrowser.HasSelected()) {
        auto selected = mFileBrowser.GetMultiSelected();
        for (const auto& file : selected) {
            mContext.load_texture(file, ImageFormat::eUnknown);
        }
        mFileBrowser.ClearSelected();
    }
}

AssetBrowserPanel::AssetBrowserPanel(render::Context& context)
    : IEditorPanel("Asset Browser")
    , mContext(context)
{
    mFileBrowser.SetTitle("Import Assets");
}
