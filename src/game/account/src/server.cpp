#include "stdafx.hpp"

#include "account/account.hpp"

#include "core/defer.hpp"

#include <fmtlib/format.h>

#include "account.dao.hpp"

using namespace std::chrono_literals;

using namespace game;
using namespace sm;

namespace acd = sm::dao::account;

static std::optional<acd::User> getUserByName(db::Connection& db, std::string_view name) try {
    // TODO: horrible evil, need to support arbitrary prepared statements
    std::string query = fmt::format("SELECT * FROM user WHERE name = '{}'", name);

    return db.selectOneWhere<acd::User>(query);
} catch (const db::DbException& e) {
    if (e.error().noData()) {
        return std::nullopt;
    }

    throw;
}

bool AccountServer::authSession(SessionId id) {
    std::lock_guard guard(mDbMutex);

    auto stmt = mAccountDb.prepareQuery("SELECT COUNT(*) FROM session WHERE id = :id");
    stmt.bind("id").to(id);
    auto result = db::throwIfFailed(stmt.start());

    return result.at<int>(0) > 0;
}

bool AccountServer::createAccount(game::CreateAccount info) {
    std::scoped_lock guard(mSaltMutex, mDbMutex);

    auto existingUser = getUserByName(mAccountDb, info.username);
    if (existingUser.has_value()) {
        LOG_WARN(GlobalLog, "account already exists: {}", info.username.text());
        return false;
    }

    std::string salt = mSalt.getSaltString(16);
    uint64_t password = hashWithSalt(info.password, salt);

    acd::User user {
        .name = std::string{info.username},
        .password = password,
        .salt = salt
    };

    auto result = mAccountDb.tryInsertReturningPrimaryKey(user);
    if (!result.has_value()) {
        LOG_WARN(GlobalLog, "failed to create account: {}", result.error());
        return false;
    }

    LOG_INFO(GlobalLog, "created account: {}", result.value());

    return true;
}

SessionId AccountServer::login(game::Login info) {
    std::lock_guard guard(mDbMutex);

    auto user = getUserByName(mAccountDb, info.username);
    if (!user.has_value()) {
        fmt::println(stderr, "user not found: `{}`", info.username.text());
        return UINT64_MAX;
    }

    uint64_t password = hashWithSalt(info.password, user->salt);
    if (user->password != password) {
        fmt::println(stderr, "invalid password for user: `{}`", info.username.text());
        return UINT64_MAX;
    }

    acd::Session session {
        .user = user->id
    };

    auto result = mAccountDb.tryInsertReturningPrimaryKey(session);
    if (!result.has_value()) {
        fmt::println(stderr, "failed to create session: {}", result.error());
        return UINT64_MAX;
    }

    fmt::println(stderr, "created session: {}", result.value());
    return result.value();
}

LobbyId AccountServer::createLobby(game::CreateLobby info) {
    std::lock_guard guard(mDbMutex);

    acd::Lobby lobby {
        .name = std::string{info.name.text()},
        .owner = info.session,
        .state = "O"
    };

    auto result = mAccountDb.tryInsertReturningPrimaryKey(lobby);
    if (!result.has_value()) {
        fmt::println(stderr, "failed to create lobby: {}", result.error());
        return UINT64_MAX;
    }

    fmt::println(stderr, "created lobby: {}", result.value());
    return result.value();
}

bool AccountServer::joinLobby(game::JoinLobby info) {
    std::lock_guard guard(mDbMutex);

    std::string_view sql = R"(
        UPDATE lobby
        SET state = 'J', client = :user
        WHERE id = :lobby AND state = 'O'
    )";

    auto stmt = mAccountDb.prepareUpdate(sql);
    stmt.bind("user").to(info.session);
    stmt.bind("lobby").to(info.lobby);
    auto result = stmt.execute();

    return result.isSuccess() && stmt.rowsAffected() > 0;
}

uint64_t AccountServer::getSessionList(std::span<SessionInfo> sessions) {
    std::lock_guard guard(mDbMutex);

    std::string_view query = R"(
        SELECT s.id, u.name
        FROM session s
        JOIN user u ON u.id = s.user
    )";
    auto result = mAccountDb.selectSql(query);

    uint64_t count = 0;
    for (auto& row : result) {
        if (count >= sessions.size())
            break;

        sessions[count++] = SessionInfo {
            .id = row.at<uint64_t>(0),
            .name = std::string_view{row.at<std::string>(1)}
        };
    }

    return count;
}

uint64_t AccountServer::getLobbyList(std::span<LobbyInfo> lobbies) try {
    std::lock_guard guard(mDbMutex);

    std::string_view query = "SELECT * FROM lobby";
    auto result = mAccountDb.selectSql(query);

    uint64_t count = 0;
    for (auto& row : result) {
        if (count >= lobbies.size())
            break;

        lobbies[count++] = LobbyInfo {
            .id = row.at<uint64_t>(0),
            .name = std::string_view{row.at<std::string>(1)}
        };
    }

    return count;
} catch (const db::DbException& e) {
    if (e.error().noData()) {
        return 0;
    }

    throw;
}

void AccountServer::dropSession(SessionId session) {
    if (session == UINT64_MAX)
        return;

    std::lock_guard guard(mDbMutex);

    auto stmt = mAccountDb.prepareUpdate("DELETE FROM session WHERE id = :id");
    stmt.bind("id").to(session);
    if (db::DbError error = stmt.execute()) {
        LOG_WARN(GlobalLog, "Failed to remove data associated with session {}. {}", session, error);
    }
}

void AccountServer::handleClient(const std::stop_token& stop, net::Socket socket) noexcept try {
    MessageRouter router;
    SessionId session = UINT64_MAX;

    defer { dropSession(session); };

    router.addRoute<Ack>(PacketType::eAck, [](const Ack& req) {
        LOG_INFO(GlobalLog, "received ack: {}/{}", req.header.id, req.header.stream);

        return Ack { req.header };
    });

    router.addRoute<CreateAccount>(PacketType::eCreateAccount, [this](const CreateAccount& req) {
        if (createAccount(req)) {
            return Response { req.header, Status::eSuccess };
        } else {
            return Response { req.header, Status::eFailure };
        }
    });

    router.addRoute<Login>(PacketType::eLogin, [this, &session](const Login& req) {
        SessionId auth = login(req);
        if (auth == UINT64_MAX) {
            return NewSession { req.header };
        } else {
            session = auth;
            return NewSession { req.header, auth };
        }
    });

    router.addRoute<CreateLobby>(PacketType::eCreateLobby, [this](const CreateLobby& req) {
        LobbyId id = createLobby(req);
        if (id == UINT64_MAX) {
            return NewLobby { req.header };
        } else {
            return NewLobby { req.header, id };
        }
    });

    router.addRoute<JoinLobby>(PacketType::eJoinLobby, [this](const JoinLobby& req) {
        if (joinLobby(req)) {
            return Response { req.header, Status::eSuccess };
        } else {
            return Response { req.header, Status::eFailure };
        }
    });

    router.addFlexibleDataRoute<GetSessionList>(PacketType::eGetSessionList, [this](const GetSessionList& req, net::Socket& socket) {
        if (!authSession(req.session)) {
            socket.send(Response { req.header, Status::eFailure }).throwIfFailed();
            return;
        }

        static constexpr size_t kMaxSessions = 256;
        std::unique_ptr<std::byte[]> data = std::make_unique<std::byte[]>(sizeof(SessionList) + (sizeof(SessionInfo) * kMaxSessions));
        SessionList *list = reinterpret_cast<SessionList*>(data.get());
        auto count = getSessionList(std::span<SessionInfo>(list->sessions, kMaxSessions));
        uint16_t size = sizeof(SessionList) + (sizeof(SessionInfo) * count);

        list->response = Response { req.header, Status::eSuccess, size };

        net::throwIfFailed(socket.sendBytes(data.get(), size));
    });

    router.addFlexibleDataRoute<GetLobbyList>(PacketType::eGetLobbyList, [this](const GetLobbyList& req, net::Socket& socket) {
        if (!authSession(req.session)) {
            socket.send(Response { req.header, Status::eFailure }).throwIfFailed();
            return;
        }

        static constexpr size_t kMaxLobbies = 256;
        std::unique_ptr<std::byte[]> data = std::make_unique<std::byte[]>(sizeof(LobbyList) + (sizeof(LobbyInfo) * kMaxLobbies));
        LobbyList *list = reinterpret_cast<LobbyList*>(data.get());
        auto count = getLobbyList(std::span<LobbyInfo>(list->lobbies, kMaxLobbies));
        uint16_t size = sizeof(LobbyList) + (sizeof(LobbyInfo) * count);

        list->response = Response { req.header, Status::eSuccess, size };

        net::throwIfFailed(socket.sendBytes(data.get(), size));
    });

    SocketMux mux;

    while (!stop.stop_requested()) {
        mux.work(socket, 100ms);

        if (AnyPacket packet = mux.pop(kClientStream)) {
            if (!router.handleMessage(packet, socket)) {
                LOG_INFO(GlobalLog, "dropping client connection");
                return;
            }
        }

        mux.pop(kEventStream); // server doesnt handle events, just keep the queue empty
    }
} catch (const db::DbException& e) {
    LOG_WARN(GlobalLog, "database error: {}", e);
} catch (const net::NetException& e) {
    LOG_WARN(GlobalLog, "network error: {}", e);
} catch (const std::exception& e) {
    LOG_ERROR(GlobalLog, "unhandled exception: {}", e.what());
} catch (...) {
    LOG_ERROR(GlobalLog, "unknown unhandled exception");
}

static void createSchema(db::Connection& db) {
    db.createTable(acd::User::table());
    db.createTable(acd::Message::table());
    db.createTable(acd::Session::table());
    db.createTable(acd::Lobby::table());

    db.truncate(acd::Session::table());
    db.truncate(acd::Lobby::table());
}

AccountServer::AccountServer(db::Connection db, net::Network& net, const net::Address& address, uint16_t port) noexcept(false)
    : AccountServer(std::move(db), net, address, port, std::random_device{}())
{ }

AccountServer::AccountServer(db::Connection db, net::Network& net, const net::Address& address, uint16_t port, unsigned seed) noexcept(false)
    : mAccountDb(std::move(db))
    , mNetwork(net)
    , mServer(mNetwork.bind(address, port))
    , mSalt(seed)
{
    createSchema(mAccountDb);
}

void AccountServer::listen(uint16_t connections) {
    begin(connections);
    work();
}

void AccountServer::begin(uint16_t connections) {
    mServer.listen(connections).throwIfFailed();
}

void AccountServer::work() {
    std::unordered_set<std::unique_ptr<std::jthread>> threads;
    while (mServer.isActive()) {
        net::NetResult<net::Socket> client = mServer.tryAccept();
        if (isCancelled(client))
            break;

        net::Socket socket = net::throwIfFailed(std::move(client));

        threads.emplace(std::make_unique<std::jthread>([&, socket = std::move(socket)](const std::stop_token& stop) mutable {
            handleClient(stop, std::move(socket));
        }));
    }
}

void AccountServer::stop() {
    mServer.cancel();
}

bool AccountServer::isRunning() const {
    return mServer.isActive();
}
