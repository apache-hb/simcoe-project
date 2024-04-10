#pragma once

#include "editor/draw.hpp"

#include "imfilebrowser.h"

namespace sm::ed {
    class AssetBrowserPanel final {
        ed::EditorContext &mContext;

        float mThumbnailSize = 64.0f;
        float mThumbnailPadding = 4.0f;

        int mActiveTab = world::eScene;

        void draw_grid(size_t count, auto&& fn) {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            uint columns = (uint)(avail.x / (mThumbnailSize + mThumbnailPadding));
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

        // IEditorPanel
        void draw_content();

        void draw_images();
        void draw_models();
        void draw_lights();
        void draw_materials();

    public:
        AssetBrowserPanel(ed::EditorContext& context);

        bool mOpen = true;
        void draw_window();
    };
}
