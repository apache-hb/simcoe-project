#pragma once

#include "editor/draw.hpp"

#include "imfilebrowser.h"

namespace sm::ed {
    struct AssetBrowser {
        ed::EditorContext &ctx;

        float thumbnailSize = 64.0f;
        float thumbnailPadding = 4.0f;

        int activeTab = world::eScene;
        bool isOpen = true;

        AssetBrowser(ed::EditorContext& context);
        void draw_window();
    };
}
