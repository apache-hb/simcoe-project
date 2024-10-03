#include "stdafx.hpp"

#include "logs/structured/logger.hpp"
#include "logs/structured/channels.hpp"

namespace structured = sm::logs::structured;

void structured::setup(db::Connection connection) {
    std::unique_ptr<IAsyncLogChannel> channel(database(std::move(connection)));
    structured::Logger::instance().addAsyncChannel(std::move(channel));
}

void structured::shutdown() {
    structured::Logger::instance().shutdown();
}
