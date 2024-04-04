#pragma once

#include "editor/draw.hpp"

namespace sm::ed {
    class InspectorPanel final {
        ed::EditorContext &mContext;

        void draw_content();

        void draw_mesh(ItemIndex index);
        void draw_node(ItemIndex index);
        void draw_camera(ItemIndex index);
        void draw_image(ItemIndex index);
        void draw_material(ItemIndex index);

    public:
        InspectorPanel(ed::EditorContext &context);

        bool mOpen = true;
        void draw_window();
    };
}
