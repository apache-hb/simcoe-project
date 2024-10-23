#pragma once

#include "test/common.hpp"

#include "render/next/context/core.hpp"
#include "render/next/surface/virtual.hpp"
#include "render/next/surface/swapchain.hpp"

using namespace sm;
using namespace sm::render;

using FrameEvent = std::function<void()>;

next::SurfaceInfo newSurfaceInfo(math::uint2 size, UINT length = 2, math::float4 clearColour = { 0.0f, 0.2f, 0.4f, 1.0f });

struct ContextTest {
    next::ISwapChainFactory *swapChainFactory;

    next::CoreContext context;
    std::map<int, FrameEvent> events;
    int iters;

    ContextTest(int frames, math::uint2 size, next::ISwapChainFactory *factory);

    void event(int frame, FrameEvent it) {
        events[frame] = std::move(it);
        iters = std::max(iters, frame);
    }

    int next();
    void run() { while (next() > 0); }
    void doEvent();
};

struct VirtualContextTest {
    next::VirtualSwapChainFactory virtualSwapChain;
    ContextTest context;

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

struct WindowContextTest {
    TestWindowEvents events{};
    std::string name;
    system::Window window;
    next::WindowSwapChainFactory windowSwapChain;
    ContextTest context;

    WindowContextTest(int frames);

    void event(int frame, FrameEvent it) {
        context.event(frame, std::move(it));
    }

    next::CoreContext& getContext() { return context.context; }
    math::uint2 getClientSize() { return window.getClientCoords().size(); }

    bool next();
};
