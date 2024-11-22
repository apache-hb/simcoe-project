#pragma once

#include "net/net.hpp"

#include "launch/guiwindow.hpp"

#include <imgui/imgui.h>

using namespace sm;

static constexpr net::Address kAddress = net::Address::loopback();
static constexpr uint16_t kPort = 9979;
