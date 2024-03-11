#pragma once

#include "render/vendor/microsoft/pix.hpp"

#include "editor/panel.hpp"

namespace sm::render {
    struct Context;
}

namespace sm::ed {
    class PixPanel final : public IEditorPanel {
        render::Context& mContext;
        PIXHUDOptions mOptions = PIX_HUD_SHOW_ON_ALL_WINDOWS;

        bool is_pix_enabled() const;

        // IEditorPanel
        void draw_content() override;

        bool draw_menu_item(const char *shortcut) override;

    public:
        PixPanel(render::Context& context);
    };
}