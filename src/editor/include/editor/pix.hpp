#pragma once

#include "render/vendor/microsoft/pix.hpp"

namespace sm::render {
    struct Context;
}

namespace sm::ed {
    class PixPanel final {
        render::Context& mContext;
        PIXHUDOptions mOptions = PIX_HUD_SHOW_ON_ALL_WINDOWS;

        bool is_pix_enabled() const;

        // IEditorPanel
        void draw_content();

    public:
        PixPanel(render::Context& context);

        bool mOpen = true;
        void draw_window();

        bool draw_menu_item(const char *shortcut);
    };
}