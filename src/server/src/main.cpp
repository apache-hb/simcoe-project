#include "stdafx.hpp"

#include "db/connection.hpp"

#include "logs/structured/channels.hpp"

#include "net/net.hpp"

#include "launch/launch.hpp"

#include "packets/packets.hpp"

#include <random>
#include <unordered_set>

#include "server.dao.hpp"

using namespace sm;

using namespace std::chrono_literals;

LOG_MESSAGE_CATEGORY(LaunchLog, "Launch");

static sm::opt<bool> gRunAsClient {
    name = "client",
    desc = "Run as a client",
    init = false
};

static constexpr net::IPv4Address kAddress = net::IPv4Address::loopback();
static constexpr uint16_t kPort = 9979;

#if 0
__attribute__((target("crc32")))
static uint32_t crc32(const std::byte *data, size_t size) {
    uint32_t crc = 0;
    while (size > sizeof(uint64_t)) {
        crc = _mm_crc32_u64(crc, *reinterpret_cast<const uint64_t *>(data));
        data += sizeof(uint64_t);
        size -= sizeof(uint64_t);
    }

    while (size >= sizeof(uint8_t)) {
        crc = _mm_crc32_u8(crc, *reinterpret_cast<const uint8_t *>(data));
        data += sizeof(uint8_t);
        size -= sizeof(uint8_t);
    }

    return crc;
}
#endif

static int clientMain() noexcept try {
    net::Network network = net::Network::create();

    auto socket = network.connect(kAddress, kPort);
    game::CreateAccountRequestPacket packet = {
        .header = {
            .type = game::PacketType::eCreateAccountRequest,
        },
        .username = "test",
        .password = "password",
    };

    socket.send(packet).throwIfFailed();

    auto maybeResponse = socket.recv<game::CreateAccountResponsePacket>();

    if (!maybeResponse) {
        LOG_WARN(GlobalLog, "failed to receive create account response: {}", maybeResponse.error());
        return -1;
    }

    auto response = maybeResponse.value();

    if (response.header.type != game::PacketType::eCreateAccountResponse) {
        LOG_WARN(GlobalLog, "unexpected packet type: {}", response.header.type);
        return -1;
    }

    LOG_INFO(GlobalLog, "received create account response: {}", response.status);

    return 0;
} catch (std::exception& err) {
    LOG_ERROR(GlobalLog, "unhandled exception in client: {}", err.what());
    return -1;
} catch (...) {
    LOG_ERROR(GlobalLog, "unknown unhandled exception in client");
    return -1;
}

static void recvDataPacket(std::span<std::byte> dst, net::Socket& socket, game::PacketHeader header) {
    std::memcpy(dst.data(), &header, sizeof(header));
    size_t size = game::getPacketDataSize(header);

    size_t read = socket.recvBytes(dst.data() + sizeof(header), size).value();
    if (read != size) {
        LOG_ERROR(GlobalLog, "failed to receive all packet data: expected {}, got {}", size, read);
    }
}

static std::string createSalt(int length) {
    static constexpr char kChars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device random{};
    std::mt19937 gen(random());

    std::uniform_int_distribution<int> dist(0, sizeof(kChars) - 1);

    std::string salt;
    salt.reserve(length);

    for (int i = 0; i < length; i++) {
        salt.push_back(kChars[dist(gen)]);
    }

    return salt;
}

static uint64_t hashWithSalt(const std::string& password, const std::string& salt) {
    std::string combined = password + salt;
    return std::hash<std::string>{}(combined);
}

static int serverMain() {
    db::Environment sqlite = db::Environment::create(db::DbType::eSqlite3);

    db::Connection users = sqlite.connect({ .host = "server-users.db" });
    users.createTable(sm::dao::server::User::table());
    users.createTable(sm::dao::server::Message::table());

    bool running = true;

    net::Network network = net::Network::create();

    auto listener = network.bind(kAddress, kPort);

    listener.listen(8).throwIfFailed();

    std::unordered_set<std::unique_ptr<std::jthread>> threads;
    std::mutex dbMutex;

    while (running) {
        auto maybeSocket = listener.tryAccept();
        if (!maybeSocket && maybeSocket.error().cancelled()) {
            break;
        }

        if (!maybeSocket) {
            LOG_ERROR(GlobalLog,"failed to accept connection: {}", maybeSocket.error());
            continue;
        }

        std::unique_ptr<std::jthread> thread = std::make_unique<std::jthread>([&, socket = std::move(maybeSocket.value())](const std::stop_token& stop) mutable {
            try {
                while (!stop.stop_requested()) {
                    auto maybeHeader = socket.recv<game::PacketHeader>();
                    if (!maybeHeader && maybeHeader.error().connectionClosed()) {
                        LOG_INFO(GlobalLog, "client disconnected");
                        break;
                    }

                    game::PacketHeader header = maybeHeader.value();

                    std::byte buffer[512];
                    recvDataPacket(buffer, socket, header);

                    if (header.type == game::PacketType::eCreateAccountRequest) {
                        auto *packet = reinterpret_cast<game::CreateAccountRequestPacket *>(buffer);
                        LOG_INFO(GlobalLog, "received create account request {} {} `{}` `{}`", packet->header.type, packet->header.crc, packet->username, packet->password);

                        std::string salt = createSalt(16);

                        sm::dao::server::User user {
                            .name = packet->username,
                            .password = hashWithSalt(packet->password, salt),
                            .salt = salt,
                        };

                        game::CreateAccountStatus result = [&] {
                            std::lock_guard guard(dbMutex);
                            db::DbError result = users.tryInsert(user);
                            return result.isSuccess() ? game::CreateAccountStatus::eSuccess : game::CreateAccountStatus::eFailure;
                        }();

                        LOG_INFO(GlobalLog, "create account result: {}", result);

                        game::CreateAccountResponsePacket resp {
                            .header = game::PacketHeader {
                                .type = game::PacketType::eCreateAccountResponse,
                            },
                            .status = result
                        };

                        socket.send(resp).throwIfFailed();
                    } else {
                        LOG_WARN(GlobalLog, "unknown packet type: {}", header.type);
                    }
                }
            } catch (std::exception& err) {
                LOG_ERROR(GlobalLog, "unhandled exception in client thread: {}", err.what());
            } catch (...) {
                LOG_ERROR(GlobalLog, "unknown unhandled exception in client thread");
            }
        });

        threads.insert(std::move(thread));
    }

    threads.clear();

    return 0;
}

static int commonMain() noexcept try {
    LOG_INFO(GlobalLog, "SMC_DEBUG = {}", SMC_DEBUG);
    LOG_INFO(GlobalLog, "CTU_DEBUG = {}", CTU_DEBUG);

    if (gRunAsClient.getValue()) {
        return clientMain();
    } else {
        return serverMain();
    }
} catch (const std::exception& err) {
    LOG_ERROR(LaunchLog, "unhandled exception: {}", err.what());
    return -1;
} catch (...) {
    LOG_ERROR(LaunchLog, "unknown unhandled exception");
    return -1;
}

static const launch::LaunchInfo kLaunchInfo {
    .logDbType = db::DbType::eSqlite3,
    .logDbConfig = { .host = "server-logs.db" },
    .logPath = "server.log",
};

int main(int argc, const char **argv) noexcept try {
    auto _ = launch::commonInit(GetModuleHandleA(nullptr), kLaunchInfo);

    sm::Span<const char*> args{argv, size_t(argc)};
    LOG_INFO(GlobalLog, "args = [{}]", fmt::join(args, ", "));

    if (int err = sm::parseCommandLine(argc, argv)) {
        return (err == -1) ? 0 : err; // TODO: this is a little silly, should wrap in a type
    }

    int result = commonMain();

    LOG_INFO(LaunchLog, "editor exiting with {}", result);

    return result;
} catch (const db::DbException& err) {
    LOG_ERROR(LaunchLog, "database error: {}", err.error());
    return -1;
} catch (const std::exception& err) {
    LOG_ERROR(LaunchLog, "unhandled exception: {}", err.what());
    return -1;
} catch (...) {
    LOG_ERROR(LaunchLog, "unknown unhandled exception");
    return -1;
}

int WinMain(HINSTANCE hInstance, SM_UNUSED HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    auto _ = launch::commonInit(GetModuleHandleA(nullptr), kLaunchInfo);

    LOG_INFO(LaunchLog, "lpCmdLine = {}", lpCmdLine);
    LOG_INFO(LaunchLog, "nShowCmd = {}", nShowCmd);
    // TODO: parse lpCmdLine

    int result = commonMain();

    LOG_INFO(LaunchLog, "editor exiting with {}", result);

    return result;
}
