#include "stdafx.hpp"

#include "account/router.hpp"

using namespace game;

namespace net = sm::net;

void MessageRouter::addGenericRoute(PacketType type, MessageHandler handler) {
    mRoutes[type] = std::move(handler);
}

bool MessageRouter::handleMessage(sm::net::Socket& socket) {
    PacketHeader header = net::throwIfFailed(socket.recv<PacketHeader>());

    uint16_t size = header.size - sizeof(PacketHeader);
    std::unique_ptr<std::byte[]> data = std::make_unique<std::byte[]>(size + sizeof(PacketHeader));
    net::throwIfFailed(socket.recvBytes(data.get() + sizeof(PacketHeader), size));

    memcpy(data.get(), &header, sizeof(PacketHeader));

    auto it = mRoutes.find(header.type);
    if (it == mRoutes.end())
        return false;

    RequestData request(data.get(), size);
    it->second(request, socket);

    return true;
}
