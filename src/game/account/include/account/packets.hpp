#pragma once

#include "packets.meta.hpp"

/**
 * Network packets for game and account actions.
 * Features 0 protection from mitm attacks or packet tampering. :^)
 */
namespace game {
    REFLECT_ENUM(PacketType)
    enum class PacketType : uint16_t {
        eInvalid = 0,

        eAck,

        eCreateAccountRequest,
        eCreateAccountResponse,

        eLoginRequest,
        eLoginResponse,

        ePostMessageRequest,
        ePostMessageResponse,

        eGetLobbyListRequest,
        eGetLobbyListResponse,

        eCreateLobbyRequest,
        eCreateLobbyResponse,

        eJoinLobbyRequest,
        eJoinLobbyResponse,

        eLeaveLobby,

        eCount
    };

    template<size_t N>
    struct VarChar {
        uint32_t length;
        char data[N];
    };

    static constexpr size_t kSessionIdSize = 32;
    using SessionId = VarChar<kSessionIdSize>;

    REFLECT()
    struct PacketHeader {
        PacketType type;
        uint16_t size;
        uint16_t id;
    };

    REFLECT_ENUM(Status)
    enum class Status : uint16_t {
        eSuccess = 0,
        eFailure = 1,
    };

    REFLECT()
    struct ResponsePacket {
        // the id in the packet header will be the same as the
        // request that instigated this response
        PacketHeader header;
        Status status;
    };

    REFLECT()
    struct AckPacket {
        PacketHeader header = {PacketType::eAck};
        uint32_t ack;
    };

    REFLECT()
    struct CreateAccountRequestPacket {
        PacketHeader header = {PacketType::eCreateAccountRequest};
        char username[32];
        char password[32];
    };

    REFLECT()
    struct CreateAccountResponsePacket {
        PacketHeader header = {PacketType::eCreateAccountResponse};
        Status status;
    };

    REFLECT()
    struct LoginRequestPacket {
        PacketHeader header = {PacketType::eLoginRequest};
        char username[32];
        char password[32];
    };

    REFLECT()
    struct LoginResponsePacket {
        PacketHeader header = {PacketType::eLoginResponse};
        Status status;
        SessionId session;
    };

    REFLECT()
    struct PostMessageRequestPacket {
        PacketHeader header = {PacketType::ePostMessageRequest};
        SessionId session;
        VarChar<256> message;
    };

    REFLECT()
    struct PostMessageResponsePacket {
        PacketHeader header = {PacketType::ePostMessageResponse};
        Status status;
    };

    REFLECT()
    struct CreateLobby {
        PacketHeader header = {PacketType::eCreateLobbyRequest};
        SessionId session;
    };

    REFLECT()
    struct JoinLobby {
        PacketHeader header = {PacketType::eJoinLobbyRequest};
        SessionId session;
    };

    size_t getPacketDataSize(PacketHeader header);
}
