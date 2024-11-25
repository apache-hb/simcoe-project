#pragma once

#include "net/net.hpp"

#include "account/packets.hpp"

#include <queue>

namespace game {
    struct AnyPacket {
        std::unique_ptr<std::byte[]> bytes;

        void *data() const { return bytes.get(); }

        PacketHeader& header() const {
            return *std::bit_cast<PacketHeader*>(bytes.get());
        }

        size_t bodySize() const noexcept {
            return header().size - sizeof(PacketHeader);
        }

        uint8_t stream() const { return header().stream; }
        operator bool() const { return !!bytes; }
    };

    AnyPacket readSinglePacket(sm::net::Socket& socket);
    AnyPacket readSinglePacket(sm::net::Socket& socket, std::chrono::milliseconds timeout);

    struct SocketPartition {
        std::queue<AnyPacket> input;
    };

    /// @brief multiplex network stream.
    /// Marhshalls multiple logical communication streams over a single physical socket.
    class SocketMux {
        static constexpr uint8_t kMaxStreams = 0b00001111;
        SocketPartition mStreams[kMaxStreams];

        SocketPartition& getPartition(uint8_t id);

    public:
        AnyPacket pop(uint8_t stream);

        void work(sm::net::Socket& socket, std::chrono::milliseconds timeout);
    };
}
