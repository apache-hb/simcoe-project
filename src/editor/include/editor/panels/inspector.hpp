#pragma once

#include "editor/draw.hpp"

namespace sm::ed {
    class InspectorPanel final {
        ed::EditorContext &mContext;

        void draw_content();

        void inspect(world::IndexOf<world::Model> index);
        void inspect(world::IndexOf<world::Node> index);
        void inspect(world::IndexOf<world::Material> index);
        void inspect(world::IndexOf<world::Image> index);
        void inspect(world::IndexOf<world::Light> index);

        template<world::IsWorldObject T>
        void inspect(world::IndexOf<T> index) {
            ImGui::TextUnformatted("Unimplemented :(");
        }

    public:
        InspectorPanel(ed::EditorContext &context);

        bool mOpen = true;
        void draw_window();
    };
}
