#pragma once

#include "net/net.hpp"

#include "logs/logging.hpp"

sm::net::NetError lastNetError();

LOG_MESSAGE_CATEGORY(NetLog, "Net");
