#include "net_test_common.hpp"

#include <thread>

#include "net/net.hpp"

using namespace sm;
using namespace sm::net;

using namespace std::chrono_literals;

static constexpr size_t kBufferSize = 128;

TEST_CASE("Network client server connection") {
    net::create();

    Network network = Network::create();

    NetTestStream errors;

    ListenSocket server = network.bind(Address::loopback(), 0);
    uint16_t port = server.getBoundPort();

    REQUIRE(server.setBlocking(false).isSuccess());
    REQUIRE(server.listen(1).isSuccess());

    std::jthread serverThread = std::jthread([&](const std::stop_token& stop) {
        while (stop.stop_requested()) {
            NetResult<Socket> client = [&] {
                std::stop_callback cb(stop, [&] { server.cancel(); });
                return server.tryAccept();
            }();

            if (!client.has_value()) {
                const auto& err = client.error();
                if (!err.cancelled())
                    errors.add("Failed to accept client: {}", err.message());

                break;
            }

            char buffer[kBufferSize];
            for (size_t i = 0; i < sizeof(buffer); ++i) {
                buffer[i] = std::hash<size_t>{}(i) % CHAR_MAX;
            }

            size_t sent = client->sendBytes(buffer, sizeof(buffer)).value();
            errors.expect(sent == sizeof(buffer), "Sent {} bytes, expected {}", sent, sizeof(buffer));
        }
    });

    Socket client = network.connect(Address::loopback(), port);
    client.setBlocking(false).throwIfFailed();
    client.setRecvTimeout(256ms).throwIfFailed();

    fmt::println(stderr, "client connected");

    // recv a buffer larger than what will ever be sent. this should timeout
    char buffer[kBufferSize * 16];
    auto [read, err] = client.recvBytesTimeout(buffer, sizeof(buffer), 256ms);

    fmt::println(stderr, "client read {} bytes", read);

    errors.expect(err.timeout(), "Expected timeout, got: {}. read {} bytes", err.message(), read);
}
