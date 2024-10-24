#pragma once

#include "test/common.hpp"

#include "render/next/context/core.hpp"
#include "render/next/surface/virtual.hpp"
#include "render/next/surface/swapchain.hpp"

using namespace sm;
using namespace sm::render;

using FrameEvent = std::function<void()>;

next::SurfaceInfo newSurfaceInfo(math::uint2 size, UINT length = 2, math::float4 clearColour = { 0.0f, 0.2f, 0.4f, 1.0f });
next::ContextConfig newConfig(next::ISwapChainFactory *factory, math::uint2 size);
system::WindowConfig newWindowConfig(const char *title);

struct FrameEvents {
    std::map<int, FrameEvent> events;
    int iters;

    FrameEvents(int frames)
        : iters(frames)
    { }

    void event(int frame, FrameEvent it) {
        events[frame] = std::move(it);
        iters = std::max(iters, frame);
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
        }
    }
};

template<typename C> requires std::is_base_of_v<next::CoreContext, C>
struct ContextTest {
    next::ISwapChainFactory *swapChainFactory;

    C context;
    FrameEvents events;

    ContextTest(int frames, math::uint2 size, next::ISwapChainFactory *factory, auto&&... args)
        : swapChainFactory(factory)
        , context(newConfig(swapChainFactory, size), args...)
        , events(frames)
    { }

    void event(int frame, FrameEvent it) {
        events.event(frame, std::move(it));
    }

    int next() {
        context.present();
        return events.next();
    }

    void run() { while (next() > 0); }

    void doEvent()  {
        events.doEvent();
    }
};

struct VirtualContextTest {
    next::VirtualSwapChainFactory virtualSwapChain;
    ContextTest<next::CoreContext> context;

    VirtualContextTest(int frames)
        : context(frames, { 800, 600 }, &virtualSwapChain)
    { }

    void event(int frame, FrameEvent it) {
        context.event(frame, std::move(it));
    }

    next::CoreContext& getContext() { return context.context; }
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

    WindowBaseTest(int frameCount)
        : name(sm::system::getProgramName())
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

        return !done && frames.iters-- > 0;
    }
};

struct WindowContextTest : WindowBaseTest<> {
    ContextTest<next::CoreContext> context;

    WindowContextTest(int frames)
        : WindowBaseTest(frames)
        , context(frames, window.getClientCoords().size(), &windowSwapChain)
    {
        window.showWindow(system::ShowWindow::eShow);
        events.context = &context.context;
    }

    next::CoreContext& getContext() { return context.context; }

    void event(int frame, FrameEvent it) {
        context.event(frame, std::move(it));
    }
};
