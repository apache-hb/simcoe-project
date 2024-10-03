#include "stdafx.hpp"

#include "logs/structured/logger.hpp"
#include "logs/structured/channels.hpp"

namespace structured = sm::logs::structured;

void structured::setup(db::Connection& connection) {
    std::unique_ptr<ILogChannel> channel(database(connection));
    structured::Logger::instance().addChannel(std::move(channel));
}

void structured::shutdown() {
    structured::Logger::instance().shutdown();
}
