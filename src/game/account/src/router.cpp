#include "stdafx.hpp"

#include "account/stream.hpp"

#include "account/router.hpp"

using namespace game;

namespace net = sm::net;

void MessageRouter::addGenericRoute(PacketType type, MessageHandler handler) {
    mRoutes[type] = std::move(handler);
}

bool MessageRouter::handleMessage(AnyPacket& packet, sm::net::Socket& socket) {
    PacketHeader& header = packet.header();

    auto it = mRoutes.find(header.type);
    if (it == mRoutes.end())
        return false;

    it->second(packet, socket);

    return true;
}
