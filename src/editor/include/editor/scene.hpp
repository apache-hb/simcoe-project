#pragma once

#include "core/core.hpp"

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

        bool mShowObjects = false;
        bool mShowCameras = false;

        world::Node mNodeInfo{.name = "Node", .transform = {.scale = 1.f}};

        sm::String mMeshName;
        //world::MeshInfo mMeshCreateInfo[world::ObjectType::kCount];

        bool begin_tree_item(ItemIndex index, ImGuiTreeNodeFlags flags);

        void draw_node(uint16 index);
        void draw_leaf(uint16 index);
        void draw_group(uint16 index);

        void draw_cameras();

        void create_primitive();

        // IEditorPanel
        void draw_content();

    public:
        ScenePanel(ed::EditorContext& context);

        bool mOpen = true;
        void draw_window();
    };
}
