#pragma once

#include "editor/draw.hpp"

namespace sm::ed {
    class InspectorPanel final {
        ed::EditorContext &mContext;

        void draw_content();

        template<world::IsWorldObject T>
        void inspect(world::IndexOf<T> index) {
            ImGui::TextUnformatted("Unimplemented :(");
        }

        void inspect(world::IndexOf<world::Model> index);
        void inspect(world::IndexOf<world::Node> index);

    public:
        InspectorPanel(ed::EditorContext &context);

        bool mOpen = true;
        void draw_window();
    };
}
