#pragma once

#include "core/core.hpp"

#include "editor/panel.hpp"

#include "world/world.hpp"

#include "imgui/imgui.h"

#include "editor.reflect.h"

namespace sm::render {
    struct Context;
}

namespace sm::ed {
    class ViewportPanel;

    using ItemType = editor::ItemType;

    struct ItemIndex {
        ItemType type;
        uint16 index;

        constexpr auto operator<=>(const ItemIndex&) const = default;
        constexpr bool has_selection() const { return type != ed::ItemType::eNone; }

        static constexpr ItemIndex none() { return {ItemType::eNone, 0}; }
    };

    class ScenePanel final : public IEditorPanel {
        render::Context& mContext;
        ViewportPanel& mViewport;
        ItemIndex mSelected = ItemIndex::none();

        bool mShowObjects = false;
        bool mShowCameras = false;

        world::NodeInfo mNodeInfo{.name = "Node", .transform = {.scale = 1.f}};

        sm::String mMeshName;
        world::MeshInfo mMeshCreateInfo[world::ObjectType::kCount];

        bool begin_tree_item(ItemIndex index, ImGuiTreeNodeFlags flags);

        void draw_node(uint16 index);
        void draw_leaf(uint16 index);
        void draw_group(uint16 index);

        void draw_cameras();

        void create_primitive();

        // IEditorPanel
        void draw_content() override;

    public:
        ScenePanel(render::Context& context, ViewportPanel& viewport);
    };
}
