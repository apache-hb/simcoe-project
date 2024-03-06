#pragma once

#include "core/core.hpp"

#include "render/editor/panel.hpp"

#include "imgui/imgui.h"

namespace sm::render {
    struct Context;
}

namespace sm::editor {
    struct ItemIndex {
        enum { eNode, eObject, eNone } type;
        uint16 index;

        constexpr auto operator<=>(const ItemIndex&) const = default;
    };

    class ScenePanel final : public IEditorPanel {
        render::Context& mContext;
        ItemIndex mSelected = {ItemIndex::eNone, 0};

        bool begin_tree_node(ItemIndex index, ImGuiTreeNodeFlags flags);

        void draw_node(uint16 index);
        void draw_leaf(uint16 index);
        void draw_group(uint16 index);

        // IEditorPanel
        void draw_content() override;

    public:
        ScenePanel(render::Context& context);
    };
}
