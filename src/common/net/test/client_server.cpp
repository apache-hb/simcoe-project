#include "core/error/error.hpp"
#include "net/error.hpp"
#include "net_test_common.hpp"

#include <array>
#include <latch>
#include <thread>

#include "net/net.hpp"

using namespace sm;
using namespace sm::net;

static const char kMessage[] = "Hello, world!";
static constexpr int kClientCount = 10;

TEST_CASE("Network client server connection") {
    try {
        net::create();

        Network network = Network::create();
        ListenSocket server = network.bind(Address::loopback(), 0);
        server.listen(32).throwIfFailed();
        uint16_t port = server.getBoundPort();

        NetTestStream errors;
        std::atomic<int> clientCount = 0;

        std::jthread serverThread = std::jthread([&](const std::stop_token& stop) {
            while (!stop.stop_requested()) {
                try {
                    auto client = [&] {
                        std::stop_callback cb(stop, [&] { server.cancel(); });
                        return server.tryAccept();
                    }();

                    if (!client.has_value()) {
                        const NetError& err = client.error();
                        if (!err.cancelled())
                            errors.add("Failed to accept client: {}", err.message());

                        break;
                    }

                    size_t sent = client->sendBytes(kMessage, sizeof(kMessage)).value();
                    if (sent != sizeof(kMessage)) {
                        errors.add("Sent {} bytes, expected {}", sent, sizeof(kMessage));
                        break;
                    }
                } catch (const errors::AnyException& e) {
                    fmt::print(stderr, "Network exception: {}\n", e.what());
                    for (const auto& frame : e.stacktrace()) {
                        fmt::print(stderr, "  {}\n", frame.description());
                    }

                    errors.add("Network exception: {}", e.what());
                }
            }
        });

        std::jthread clientThread = std::jthread([&] {
            try {
                for (int i = 0; i < kClientCount; ++i) {
                    Socket client = network.connect(Address::loopback(), port);

                    std::array<char, 1024> buffer;
                    size_t received = client.recvBytes(buffer.data(), buffer.size()).value();
                    if (received != sizeof(kMessage)) {
                        errors.add("Received {} bytes, expected {}", received, sizeof(kMessage));
                    }

                    if (std::memcmp(buffer.data(), kMessage, sizeof(kMessage)) != 0) {
                        errors.add("Received unexpected data: {}", std::string_view{buffer.data(), received});
                    }

                    clientCount += 1;
                }
            } catch (const errors::AnyException& e) {
                fmt::print(stderr, "Network exception: {}\n", e.what());
                for (const auto& frame : e.stacktrace()) {
                    fmt::print(stderr, "  {}\n", frame.description());
                }

                errors.add("Network exception: {}", e.what());
            }
        });

        clientThread.join();

        serverThread.request_stop();
        serverThread.join();

        CHECK(clientCount == kClientCount);
    } catch (const errors::AnyException& e) {
        fmt::print(stderr, "Network exception: {}\n", e.what());
        for (const auto& frame : e.stacktrace()) {
            fmt::print(stderr, "  {}\n", frame.description());
        }

        FAIL("Network exception");
    }
}
