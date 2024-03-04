#pragma once

#include "render/editor/logger.hpp"
#include "render/editor/config.hpp"

namespace sm::render {
    struct Context;
}

namespace sm::editor {
    class Editor {
        render::Context &mContext;

        LoggerPanel& mLogger;
        RenderConfig mConfig;

        bool mShowDemo = true;
        bool mShowPlotDemo = true;

        void draw_mainmenu();
        void draw_dockspace();

    public:
        Editor(render::Context &context);

        void draw();

        render::Context& get_context() { return mContext; }
    };
}
