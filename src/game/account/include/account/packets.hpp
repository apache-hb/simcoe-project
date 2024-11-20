#pragma once

#include "account/guid.hpp"

/**
 * Network packets for game and account actions.
 * Features 0 protection from mitm attacks or packet tampering. :^)
 */
namespace game {
    enum class PacketType : uint16_t {
        eInvalid = 0,

        eResponse,
        eAck,

        eCreateAccount,
        eLogin,

        ePostMessage,

        eGetSessionList,
        eGetSessionInfo,

        eGetLobbyList,
        eGetLobbyInfo,

        eGetUserList,
        eGetUserInfo,

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

    using SessionId = uint64_t;
    using LobbyId = uint64_t;

    struct PacketHeader {
        PacketType type;
        uint16_t size;
        uint16_t id;
        uint8_t padding[2];
    };

    enum class Status : uint16_t {
        eSuccess = 0,
        eFailure = 1,
    };

    struct Response {
        // the id in the packet header will be the same as the
        // request that instigated this response
        PacketHeader header;

        Status status;

        Response() = default;

        Response(uint16_t id, Status status, uint16_t size = sizeof(Response))
            : header(PacketType::eResponse, size, id)
            , status(status)
        { }
    };

    struct Ack {
        PacketHeader header;

        Ack() = default;

        Ack(uint16_t id)
            : header(PacketType::eAck, sizeof(Ack), id)
        { }
    };

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

    struct NewSession {
        Response response;
        SessionId session;

        NewSession() = default;

        NewSession(uint16_t id, SessionId session)
            : NewSession(id, Status::eSuccess, session)
        { }

        NewSession(uint16_t id)
            : NewSession(id, Status::eFailure, 0)
        { }

        NewSession(uint16_t id, Status status, SessionId session)
            : response(id, status, sizeof(NewSession))
            , session(session)
        { }
    };

    struct CreateLobby {
        PacketHeader header;
        SessionId session;
        Text<32> name;

        CreateLobby() = default;

        CreateLobby(uint16_t id, SessionId session, std::string_view name)
            : header(PacketType::eCreateLobby, sizeof(CreateLobby), id)
            , session(session)
            , name(name)
        { }
    };

    struct NewLobby {
        Response response;
        LobbyId lobby;

        NewLobby() = default;

        NewLobby(uint16_t id, LobbyId lobby)
            : response(id, Status::eSuccess, sizeof(NewLobby))
            , lobby(lobby)
        { }

        NewLobby(uint16_t id)
            : response(id, Status::eFailure, sizeof(NewLobby))
            , lobby(UINT64_MAX)
        { }
    };

    struct JoinLobby {
        PacketHeader header;
        SessionId session;
        LobbyId lobby;

        JoinLobby() = default;

        JoinLobby(uint16_t id, SessionId session, LobbyId lobby)
            : header(PacketType::eJoinLobby, sizeof(JoinLobby), id)
            , session(session)
            , lobby(lobby)
        { }
    };

    struct GetSessionList {
        PacketHeader header;
        SessionId session;

        GetSessionList() = default;

        GetSessionList(uint16_t id, SessionId session)
            : header(PacketType::eGetSessionList, sizeof(GetSessionList), id)
            , session(session)
        { }
    };

    struct SessionInfo {
        SessionId id;
        Text<32> name;
    };

    struct SessionList {
        Response response;
        SessionInfo sessions[];
    };

    struct GetLobbyList {
        PacketHeader header;
        SessionId session;

        GetLobbyList() = default;

        GetLobbyList(uint16_t id, SessionId session)
            : header(PacketType::eGetLobbyList, sizeof(GetLobbyList), id)
            , session(session)
        { }
    };

    struct LobbyInfo {
        LobbyId id;
        SessionId players[4];
        Text<32> name;

        SessionId getHost() const {
            return players[0];
        }

        int getPlayerCount() const {
            int count = 0;
            for (SessionId id : players) {
                if (id != UINT64_MAX)
                    count++;
            }

            return count;
        }
    };

    struct LobbyList {
        Response response;
        LobbyInfo lobbies[];
    };

    size_t getPacketDataSize(PacketHeader header);
}
