#include "net_test_common.hpp"

#include <thread>
#include <latch>

#include "net/net.hpp"

using namespace sm;
using namespace sm::net;

using namespace std::chrono_literals;

static constexpr size_t kBufferSize = 128;
static constexpr uint16_t kPort = 9919;

TEST_CASE("Network connect timeout") {
    net::create();

    auto network = Network::create();

    NetTestStream errors;

    auto server = network.bind(Address::loopback(), kPort);

    REQUIRE(server.setBlocking(false).isSuccess());
    REQUIRE(server.setBlocking(true).isSuccess());
    REQUIRE(server.listen(1).isSuccess());

    std::jthread serverThread = std::jthread([&](const std::stop_token& stop) {
        while (stop.stop_requested()) {
            auto client = [&] {
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

    auto client = network.connect(Address::loopback(), kPort);
    client.setBlocking(false).throwIfFailed();

    int clients = 25;
    std::latch latch{clients};
    std::vector<std::jthread> workers;

    for (int i = 0; i < clients; ++i) {
        workers.emplace_back([&] {
            latch.arrive_and_wait();
            try {
                auto client = network.connectWithTimeout(Address::loopback(), kPort, 25ms);
            } catch (const NetException& err) {
                if (err.error().timeout()) {
                    return;
                }

                errors.expect(err.error().timeout(), "Expected timeout, got: {}", err.error());
            }
        });
    }

    workers.clear();

    // recv a buffer larger than what will ever be sent. this should timeout
    char buffer[kBufferSize * 16];
    auto [read, err] = client.recvBytesTimeout(buffer, sizeof(buffer), 256ms);

    errors.expect(err.timeout(), "Expected timeout, got: {}. read {} bytes", err.message(), read);
}
