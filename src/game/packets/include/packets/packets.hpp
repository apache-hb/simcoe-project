#pragma once

#include "packets.meta.hpp"

namespace game {
    REFLECT_ENUM(PacketType)
    enum class PacketType : uint32_t {
        eAck = 0,

        eCreateAccountRequest,
        eCreateAccountResponse,

        eLoginRequest,
        eLoginResponse,

        eMessageRequest,
        eMessageResponse,
    };

    template<size_t N>
    struct StringData {
        char data[N];
    };

    REFLECT()
    struct PacketHeader {
        PacketType type;
        uint32_t crc;
    };

    REFLECT()
    struct AckPacket {
        PacketHeader header;
        uint32_t ack;
    };

    REFLECT()
    struct CreateAccountRequestPacket {
        PacketHeader header;
        char username[32];
        char password[32];
    };

    REFLECT()
    struct CreateAccountResponsePacket {
        PacketHeader header;
        uint32_t result;
    };

    REFLECT()
    struct LoginRequestPacket {
        PacketHeader header;
        char username[32];
        char password[32];
    };

    REFLECT()
    struct LoginResponsePacket {
        PacketHeader header;
        uint32_t result;
    };

    REFLECT()
    struct MessageRequestPacket {
        PacketHeader header;
        char message[256];
    };

    REFLECT_ENUM(MessagePostStatus)
    enum class MessagePostStatus : uint32_t {
        eSuccess = 0,
        eFailure,
    };

    REFLECT()
    struct MessageResponsePacket {
        PacketHeader header;
        MessagePostStatus result;
    };

    static constexpr size_t getPacketDataSize(PacketHeader header) {
        switch (header.type) {
        case PacketType::eAck: return sizeof(AckPacket);
        case PacketType::eCreateAccountRequest: return sizeof(CreateAccountRequestPacket);
        case PacketType::eCreateAccountResponse: return sizeof(CreateAccountResponsePacket);
        case PacketType::eLoginRequest: return sizeof(LoginRequestPacket);
        case PacketType::eLoginResponse: return sizeof(LoginResponsePacket);
        case PacketType::eMessageRequest: return sizeof(MessageRequestPacket);
        case PacketType::eMessageResponse: return sizeof(MessageResponsePacket);

        default: return 0;
        }
    }
}
