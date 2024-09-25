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
        PacketHeader header = {PacketType::eAck, 0};
        uint32_t ack;
    };

    REFLECT()
    struct CreateAccountRequestPacket {
        PacketHeader header = {PacketType::eCreateAccountRequest, 0};
        char username[32];
        char password[32];
    };

    REFLECT_ENUM(CreateAccountStatus)
    enum class CreateAccountStatus : uint32_t {
        eSuccess = 0,
        eFailure,
    };

    REFLECT()
    struct CreateAccountResponsePacket {
        PacketHeader header = {PacketType::eCreateAccountResponse, 0};
        CreateAccountStatus status;
    };

    REFLECT()
    struct LoginRequestPacket {
        PacketHeader header = {PacketType::eLoginRequest, 0};
        char username[32];
        char password[32];
    };

    REFLECT()
    struct LoginResponsePacket {
        PacketHeader header = {PacketType::eLoginResponse, 0};
        uint32_t result;
    };

    REFLECT()
    struct MessageRequestPacket {
        PacketHeader header = {PacketType::eMessageRequest, 0};
        char message[256];
    };

    REFLECT_ENUM(MessagePostStatus)
    enum class MessagePostStatus : uint32_t {
        eSuccess = 0,
        eFailure,
    };

    REFLECT()
    struct MessageResponsePacket {
        PacketHeader header = {PacketType::eMessageResponse, 0};
        MessagePostStatus result;
    };

    static constexpr size_t getPacketDataSize(PacketHeader header) {
        size_t fullSize = [type = header.type] {
            switch (type) {
            case PacketType::eAck: return sizeof(AckPacket);
            case PacketType::eCreateAccountRequest: return sizeof(CreateAccountRequestPacket);
            case PacketType::eCreateAccountResponse: return sizeof(CreateAccountResponsePacket);
            case PacketType::eLoginRequest: return sizeof(LoginRequestPacket);
            case PacketType::eLoginResponse: return sizeof(LoginResponsePacket);
            case PacketType::eMessageRequest: return sizeof(MessageRequestPacket);
            case PacketType::eMessageResponse: return sizeof(MessageResponsePacket);

            default: return sizeof(PacketHeader);
            }
        }();

        return fullSize - sizeof(PacketHeader);
    }
}
