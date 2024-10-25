#pragma once

#include "test/common.hpp"

#include "render/next/context/core.hpp"
#include "render/next/surface/virtual.hpp"
#include "render/next/surface/swapchain.hpp"

using namespace sm;
using namespace sm::render;

using FrameEvent = std::function<void()>;

next::SurfaceInfo newSurfaceInfo(math::uint2 size, UINT length = 2, math::float4 clearColour = { 0.0f, 0.2f, 0.4f, 1.0f });
next::ContextConfig newConfig(next::ISwapChainFactory *factory, math::uint2 size, bool debug = true);
system::WindowConfig newWindowConfig(const char *title);

struct FrameEvents {
    std::map<int, FrameEvent> events;
    int iters;

    size_t executed = 0;

    FrameEvents(int frames)
        : iters(frames)
    { }

    ~FrameEvents() {
        CHECK(executed == events.size());
        CHECK(iters <= 0);
    }

    void event(int frame, FrameEvent it) {
        CTASSERTF(!events.contains(frame), "Frame %d already has an event", frame);

        iters = std::max(iters, frame);
        events[frame] = std::move(it);
    }

    int next() {
        doEvent();

        return iters--;
    }

    void run() { while (next() > 0); }

    void doEvent()  {
        auto action = events.find(iters);
        if (action != events.end()) {
            action->second();
            executed += 1;
        }
    }
};

template<typename C> requires std::is_base_of_v<next::CoreContext, C>
struct ContextTest {
    next::ISwapChainFactory *swapChainFactory;

    C context;

    ContextTest(math::uint2 size, next::ISwapChainFactory *factory, bool debug = true, auto&&... args)
        : swapChainFactory(factory)
        , context(newConfig(swapChainFactory, size, debug), args...)
    { }
};

struct VirtualContextTest {
    next::VirtualSwapChainFactory virtualSwapChain;
    ContextTest<next::CoreContext> context;
    FrameEvents frames;

    VirtualContextTest(int frameCount, bool debug = true)
        : context({ 800, 600 }, &virtualSwapChain, debug)
        , frames(frameCount)
    { }

    void event(int frame, FrameEvent it) {
        frames.event(frame, std::move(it));
    }

    next::CoreContext& getContext() { return context.context; }
    void run() { while (frames.next() > 0); }
    void doEvent() { frames.doEvent(); }
};

class TestWindowEvents final : public system::IWindowEvents {
    void resize(system::Window& window, math::int2 size) override {
        if (context == nullptr)
            return;

        next::SurfaceInfo info = newSurfaceInfo(math::uint2(size));
        context->updateSwapChain(info);
    }

public:
    next::CoreContext *context = nullptr;
};

template<typename E = TestWindowEvents> requires (std::is_base_of_v<system::IWindowEvents, E>)
struct WindowBaseTest {
    E events{};
    std::string name;
    system::Window window;
    next::WindowSwapChainFactory windowSwapChain;
    FrameEvents frames;

    WindowBaseTest(int frameCount, std::string name = sm::system::getProgramName())
        : name(std::move(name))
        , window(newWindowConfig(name.c_str()), events)
        , windowSwapChain(window.getHandle())
        , frames(frameCount)
    {
        window.showWindow(system::ShowWindow::eShow);
    }

    math::uint2 getClientSize() { return window.getClientCoords().size(); }

    bool next() {
        MSG msg = {};
        bool done = false;
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
            if (msg.message == WM_QUIT) {
                done = true;
            }
        }

        return !done && frames.next() > 0;
    }

    void event(int frame, FrameEvent it) {
        frames.event(frame, std::move(it));
    }

    void doEvent() { frames.doEvent(); }
};

struct WindowContextTest : WindowBaseTest<> {
    ContextTest<next::CoreContext> context;

    WindowContextTest(int frameCount)
        : WindowBaseTest(frameCount)
        , context(window.getClientCoords().size(), &windowSwapChain)
    {
        window.showWindow(system::ShowWindow::eShow);
        events.context = &context.context;
    }

    next::CoreContext& getContext() { return context.context; }
    void run() { while (next()); }
};
