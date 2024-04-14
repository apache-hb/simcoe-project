#pragma once

#include "editor/draw.hpp"

#include "imgui_memory_editor.h"

namespace sm::ed {
    struct Inspector {
        ed::EditorContext &ctx;
        bool isOpen = true;

        sm::HashMap<world::IndexOf<world::Buffer>, MemoryEditor> memoryEditors;

        Inspector(ed::EditorContext &context);

        void draw_window();
    };
}
