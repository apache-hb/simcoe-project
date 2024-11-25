#include "account/stream.hpp"

#include "logs/logging.hpp"

using namespace game;
using namespace sm;

AnyPacket game::readSinglePacket(sm::net::Socket& socket) {
    net::NetResult<PacketHeader> maybeHeader = socket.recv<PacketHeader>();
    if (!maybeHeader.has_value()) {
        return AnyPacket { };
    }

    PacketHeader header = maybeHeader.value();

    std::unique_ptr<std::byte[]> data = std::make_unique<std::byte[]>(header.size);
    size_t remaining = header.size - sizeof(PacketHeader);
    size_t size = net::throwIfFailed(socket.recvBytes(data.get() + sizeof(PacketHeader), remaining));
    if (size != remaining) {
        throw net::NetException{SNET_END_OF_PACKET, "expected {} bytes, sent {}", remaining, size};
    }

    memcpy(data.get(), &header, sizeof(PacketHeader));

    return AnyPacket { std::move(data) };
}

AnyPacket game::readSinglePacket(sm::net::Socket& socket, std::chrono::milliseconds timeout) {
    net::NetResult<PacketHeader> maybeHeader = socket.recvTimed<PacketHeader>(timeout);
    if (!maybeHeader.has_value()) {
        return AnyPacket { };
    }

    PacketHeader header = maybeHeader.value();

    std::unique_ptr<std::byte[]> data = std::make_unique<std::byte[]>(header.size);
    size_t remaining = header.size - sizeof(PacketHeader);
    auto [size, err] = socket.recvBytesTimeout(data.get() + sizeof(PacketHeader), remaining, timeout);
    if (size != remaining) {
        throw net::NetException{err, "Torn read recovery unimplemented, discard the connection. {} of {} bytes read.", size, remaining};
    }

    memcpy(data.get(), &header, sizeof(PacketHeader));

    return AnyPacket { std::move(data) };
}

SocketPartition& SocketMux::getPartition(uint8_t id) {
    return mStreams[id & kMaxStreams];
}

AnyPacket SocketMux::pop(uint8_t stream) {
    auto& queue = getPartition(stream).input;
    if (queue.empty()) {
        return AnyPacket { };
    }

    AnyPacket buffer = std::move(queue.front());
    queue.pop();
    return buffer;
}

void SocketMux::work(sm::net::Socket& socket, std::chrono::milliseconds timeout) {
    if (AnyPacket packet = readSinglePacket(socket, timeout)) {
        getPartition(packet.stream()).input.push(std::move(packet));
    }
}
