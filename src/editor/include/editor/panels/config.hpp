#pragma once

namespace sm::ed {
    struct EditorContext;

    class RenderConfig final {
        ed::EditorContext& mContext;

        // IEditorPanel
        void draw_content();

        void draw_adapters() const;
        void draw_allocator_info() const;
        bool draw_debug_flags() const;

    public:
        RenderConfig(EditorContext& context)
            : mContext(context)
        { }

        bool mOpen = true;
        void draw_window();
    };
}
