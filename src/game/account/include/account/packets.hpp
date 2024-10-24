#pragma once

#include "packets.meta.hpp"

namespace game {
    REFLECT_ENUM(PacketType)
    enum class PacketType : uint32_t {
        eInvalid = 0,

        eAck,

        eCreateAccountRequest,
        eCreateAccountResponse,

        eLoginRequest,
        eLoginResponse,

        ePostMessageRequest,
        ePostMessageResponse,
    };

    template<size_t N>
    struct StringData {
        uint32_t length;
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
        eFailure = 1,
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

    REFLECT_ENUM(LoginResult)
    enum class LoginResult : uint32_t {
        eSuccess,
        eFailure,
    };

    REFLECT()
    struct LoginResponsePacket {
        PacketHeader header = {PacketType::eLoginResponse, 0};
        LoginResult result;
    };

    REFLECT()
    struct PostMessageRequestPacket {
        PacketHeader header = {PacketType::ePostMessageRequest, 0};
        char message[256];
    };

    REFLECT_ENUM(MessagePostStatus)
    enum class MessagePostStatus : uint32_t {
        eSuccess,
        eFailure,
    };

    REFLECT()
    struct PostMessageResponsePacket {
        PacketHeader header = {PacketType::ePostMessageResponse, 0};
        MessagePostStatus result;
    };

    size_t getPacketDataSize(PacketHeader header);
}
