#include "stdafx.hpp"

#include "account/packets.hpp"

using namespace game;

static size_t getPacketSize(PacketHeader header) {
    switch (header.type) {
    case PacketType::eAck: return sizeof(AckPacket);
    case PacketType::eCreateAccountRequest: return sizeof(CreateAccountRequestPacket);
    case PacketType::eCreateAccountResponse: return sizeof(CreateAccountResponsePacket);
    case PacketType::eLoginRequest: return sizeof(LoginRequestPacket);
    case PacketType::eLoginResponse: return sizeof(LoginResponsePacket);
    case PacketType::ePostMessageRequest: return sizeof(PostMessageRequestPacket);
    case PacketType::ePostMessageResponse: return sizeof(PostMessageResponsePacket);

    default: return sizeof(PacketHeader);
    }
}

size_t game::getPacketDataSize(PacketHeader header) {
    return getPacketSize(header) - sizeof(PacketHeader);
}
