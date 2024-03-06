#include "render/editor/assets.hpp"

#include "render/render.hpp"

using namespace sm;
using namespace sm::editor;

using ReflectImageType = ctu::TypeInfo<ImageFormat>;

void AssetBrowserPanel::draw_images() {
    auto& textures = mContext.mTextures;

    ImVec2 avail = ImGui::GetContentRegionAvail();
    int columns = (int)(avail.x / (mThumbnailSize + mThumbnailPadding));
    if (columns < 1) columns = 1;

    for (size_t i = 0; i < textures.size(); i++) {
        const auto& texture = textures[i];
        const auto& name = texture.name;
        auto gpu = mContext.mSrvPool.gpu_handle(texture.srv);
        if (ImGui::ImageButton(name.c_str(), (ImTextureID)gpu.ptr, math::float2(mThumbnailSize))) {

        }
        ImGui::TextWrapped("%s", name.c_str());

        if ((i + 1) % columns != 0) {
            ImGui::SameLine();
        }
    }
}

void AssetBrowserPanel::draw_content() {
    if (ImGui::Button("Import")) {
        mFileBrowser.Open();
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
