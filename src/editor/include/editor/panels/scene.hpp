#pragma once

#include "editor/draw.hpp"

#include "world/world.hpp"

#include "imgui/imgui.h"

namespace sm::render {
    struct Context;
}

namespace sm::ed {
    class ViewportPanel;

    class ScenePanel final {
        ed::EditorContext& mContext;

        bool begin_tree_item(world::AnyIndex index, ImGuiTreeNodeFlags flags);

        void node_context_popup(world::IndexOf<world::Node> index);

        void draw_leaf(world::IndexOf<world::Node> index);
        void draw_group(world::IndexOf<world::Node> index);
        void draw_node(world::IndexOf<world::Node> index);

        world::IndexOf<world::Scene> scene_select();
        void draw_menu();
        void draw_content();

    public:
        ScenePanel(ed::EditorContext& context);

        bool mOpen = true;
        void draw_window();
    };
}
