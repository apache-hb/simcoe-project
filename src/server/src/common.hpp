#pragma once

#include "net/net.hpp"

#include "launch/appwindow.hpp"

#include "draw/next/context.hpp"

#include <imgui/imgui.h>

using namespace sm;

static constexpr net::Address kAddress = net::Address::loopback();
static constexpr uint16_t kPort = 9979;

class GuiWindow : public launch::AppWindow {
    draw::next::DrawContext mContext;

    void begin() override;
    void end() override;

    render::next::CoreContext& getContext() override { return mContext; }

public:
    GuiWindow(const std::string& title);
};
