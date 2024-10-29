#pragma once

#include "test/render_test_common.hpp"

#include "draw/next/context.hpp"

using namespace sm::draw::next;

struct ImGuiWindowEvents : public TestWindowEvents {
    LRESULT event(system::Window& window, UINT message, WPARAM wparam, LPARAM lparam) override;
};

struct DrawWindowTestContext : WindowBaseTest<ImGuiWindowEvents> {
    ContextTest<DrawContext> context;

    DrawWindowTestContext(int frames, bool debug = true, std::string name = sm::system::getProgramName())
        : WindowBaseTest(frames, std::move(name))
        , context(window.getClientCoords().size(), &windowSwapChain, debug, window.getHandle(), math::uint2{ 720, 480 })
    {
        window.showWindow(system::ShowWindow::eShow);
        events.context = &context.context;
    }

    DrawContext& getContext() { return context.context; }

    void update() {
        auto& ctx = getContext();
        ctx.begin();
        ctx.end();

        MSG msg = {};
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }
};
