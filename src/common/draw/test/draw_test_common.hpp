#pragma once

#include "test/render_test_common.hpp"

#include "draw/next/context.hpp"

using namespace sm::draw::next;

struct DrawWindowTestContext : WindowBaseTest<> {
    ContextTest<DrawContext> context;

    DrawWindowTestContext(int frames, bool debug = true)
        : WindowBaseTest(frames)
        , context(window.getClientCoords().size(), &windowSwapChain, debug, window.getHandle())
    {
        window.showWindow(system::ShowWindow::eShow);
        events.context = &context.context;
    }

    DrawContext& getContext() { return context.context; }
};
