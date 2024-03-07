#pragma once

#include "core/core.hpp"

#include "render/editor/panel.hpp"

#include "imgui/imgui.h"

#include "editor.reflect.h"

namespace sm::render {
    struct Context;
}

namespace sm::editor {
    class ViewportPanel;

    struct ItemIndex {
        ItemType type;
        uint16 index;

        constexpr auto operator<=>(const ItemIndex&) const = default;
        constexpr bool has_selection() const { return type != ItemType::eNone; }

        static constexpr ItemIndex none() { return {ItemType::eNone, 0}; }
    };

    class ScenePanel final : public IEditorPanel {
        render::Context& mContext;
        ViewportPanel& mViewport;
        ItemIndex mSelected = ItemIndex::none();

        bool begin_tree_node(ItemIndex index, ImGuiTreeNodeFlags flags);

        void draw_node(uint16 index);
        void draw_leaf(uint16 index);
        void draw_group(uint16 index);

        // IEditorPanel
        void draw_content() override;

    public:
        ScenePanel(render::Context& context, ViewportPanel& viewport);
    };
}
