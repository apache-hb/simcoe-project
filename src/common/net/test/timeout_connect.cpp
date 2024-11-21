#include "net_test_common.hpp"

#include <thread>
#include <latch>

#include "net/net.hpp"

using namespace sm;
using namespace sm::net;

using namespace std::chrono_literals;

static constexpr size_t kBufferSize = 128;

TEST_CASE("Network connect timeout") {
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

            size_t sent = net::throwIfFailed(client->sendBytes(buffer, sizeof(buffer)));
            errors.expect(sent == sizeof(buffer), "Sent {} bytes, expected {}", sent, sizeof(buffer));
        }
    });

    Socket client = network.connect(Address::loopback(), port);
    client.setBlocking(false).throwIfFailed();
    client.setRecvTimeout(256ms).throwIfFailed();

    int clients = 25;
    std::latch latch{clients};
    std::vector<std::jthread> workers;

    for (int i = 0; i < clients; ++i) {
        workers.emplace_back([&, i] {
            latch.arrive_and_wait();
            try {
                Socket client = network.connectWithTimeout(Address::loopback(), port, 25ms);
            } catch (const NetException& err) {
                fmt::println(stderr, "Client failed to connect {}: {}", i, err.error());

                errors.expect(err.error().timeout(), "Expected timeout, got: {}", err.error());
                return;
            }

            fmt::println(stderr, "Client connected");
        });
    }

    fmt::println(stderr, "Waiting for clients to connect");

    workers.clear();

    fmt::println(stderr, "All clients done");

    // recv a buffer larger than what will ever be sent. this should timeout
    char buffer[kBufferSize * 16];
    auto [read, err] = client.recvBytesTimeout(buffer, sizeof(buffer), 256ms);

    errors.expect(err.timeout(), "Expected timeout, got: {}. read {} bytes", err.message(), read);
}
