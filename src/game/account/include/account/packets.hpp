#pragma once

#include "account/guid.hpp"

#include "packets.meta.hpp"

/**
 * Network packets for game and account actions.
 * Features 0 protection from mitm attacks or packet tampering. :^)
 */
namespace game {
    REFLECT_ENUM(PacketType)
    enum class PacketType : uint16_t {
        eInvalid = 0,

        eResponse,
        eAck,

        eCreateAccount,
        eLogin,

        ePostMessage,
        eGetLobbyList,
        eGetUserList,

        eCreateLobby,
        eJoinLobby,
        eLeaveLobby,

        eCount
    };

    template<size_t N>
    struct Text {
        char data[N] = {};

        std::string_view text() const {
            size_t length = strnlen(data, N);
            return { data, length };
        }

        operator std::string_view() const {
            return text();
        }

        Text() = default;

        Text(std::string_view text) {
            std::memcpy(data, text.data(), std::min(text.size(), N));
        }
    };

    using SessionId = Guid;
    using LobbyId = Guid;

    REFLECT()
    struct PacketHeader {
        PacketType type;
        uint16_t size;
        uint16_t id;
        uint8_t padding[2];
    };

    REFLECT_ENUM(Status)
    enum class Status : uint16_t {
        eSuccess = 0,
        eFailure = 1,
    };

    REFLECT()
    struct Response {
        // the id in the packet header will be the same as the
        // request that instigated this response
        PacketHeader header;

        Status status;

        Response() = default;

        Response(uint16_t id, Status status, size_t size = sizeof(Response))
            : header(PacketType::eResponse, size, id)
            , status(status)
        { }
    };

    REFLECT()
    struct Ack {
        PacketHeader header;

        Ack() = default;

        Ack(uint16_t id)
            : header(PacketType::eAck, sizeof(Ack), id)
        { }
    };

    REFLECT()
    struct CreateAccount {
        PacketHeader header;
        Text<32> username;
        Text<32> password;

        CreateAccount() = default;

        CreateAccount(uint16_t id, std::string_view name, std::string_view pass)
            : header(PacketType::eCreateAccount, sizeof(CreateAccount), id)
            , username(name)
            , password(pass)
        { }

        std::string_view getUsername() const {
            return username.text();
        }

        std::string_view getPassword() const {
            return password.text();
        }
    };

    REFLECT()
    struct Login {
        PacketHeader header;
        Text<32> username;
        Text<32> password;

        Login() = default;

        Login(uint16_t id, std::string_view name, std::string_view pass)
            : header(PacketType::eLogin, sizeof(Login), id)
            , username(name)
            , password(pass)
        { }

        std::string_view getUsername() const {
            return username.text();
        }

        std::string_view getPassword() const {
            return password.text();
        }
    };

    REFLECT()
    struct NewSession {
        Response response;
        SessionId session;

        NewSession() = default;

        NewSession(uint16_t id, SessionId session)
            : NewSession(id, Status::eSuccess, session)
        { }

        NewSession(uint16_t id, Status status, SessionId session)
            : response(id, status, sizeof(NewSession))
            , session(session)
        { }
    };

    REFLECT()
    struct PostMessage {
        PacketHeader header;
        SessionId session;
        Text<256> message;

        PostMessage() = default;

        PostMessage(uint16_t id, SessionId session, std::string_view msg)
            : header(PacketType::ePostMessage, sizeof(PostMessage), id)
            , session(session)
            , message(msg)
        { }
    };

    REFLECT()
    struct CreateLobby {
        PacketHeader header;
        SessionId session;
    };

    REFLECT()
    struct NewLobby {
        Response response;
        LobbyId lobby;
    };

    REFLECT()
    struct JoinLobby {
        PacketHeader header;
        SessionId session;
        LobbyId lobby;
    };

    size_t getPacketDataSize(PacketHeader header);
}
