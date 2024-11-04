#include "stdafx.hpp"

#include "account/packets.hpp"

using namespace game;

static size_t getPacketSize(PacketHeader header) {
    return header.size;
}

size_t game::getPacketDataSize(PacketHeader header) {
    return getPacketSize(header) - sizeof(PacketHeader);
}
