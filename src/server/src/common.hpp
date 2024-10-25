#pragma once

#include "net/net.hpp"

#include "draw/next/context.hpp"
#include "render/next/surface/swapchain.hpp"

#include <imgui/imgui.h>

using namespace sm;

static constexpr net::Address kAddress = net::Address::loopback();
static constexpr uint16_t kPort = 9979;

class GuiWindowEvents : public system::IWindowEvents {
    draw::next::DrawContext *mContext = nullptr;

    LRESULT event(system::Window& window, UINT message, WPARAM wparam, LPARAM lparam) override;
    void resize(system::Window& window, math::int2 size) override;

public:
    void setContext(draw::next::DrawContext *context) {
        mContext = context;
    }
};

class GuiWindow {
    GuiWindowEvents mEvents;
    system::Window mWindow;
    render::next::WindowSwapChainFactory mSwapChain;
    draw::next::DrawContext mContext;

public:
    GuiWindow(const char *title);

    bool next();

    void present();
};
