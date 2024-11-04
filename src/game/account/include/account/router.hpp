#pragma once

#include "account/packets.hpp"

#include "net/net.hpp"

#include <unordered_map>
#include <functional>

namespace game {
    using RequestData = std::span<const std::byte>;
    using ResponseChannel = sm::net::Socket&;
    using MessageHandler = std::function<void(RequestData request, ResponseChannel response)>;

    class MessageRouter {
        std::unordered_map<PacketType, MessageHandler> mRoutes;

        void addGenericRoute(PacketType type, MessageHandler handler);

    public:
        template<typename T, typename F>
        void addRoute(PacketType type, F&& handler) {
            using ResponseType = decltype(handler(std::declval<const T&>()));

            auto messageHandler = [handler](RequestData request, ResponseChannel response) {
                const T *requestPacket = std::bit_cast<const T*>(request.data());
                if constexpr (std::is_void_v<ResponseType>) {
                    handler(*requestPacket);
                } else {
                    ResponseType responsePacket = handler(*requestPacket);
                    response.send(responsePacket).throwIfFailed();
                }
            };

            addGenericRoute(type, messageHandler);
        }

        bool handleMessage(sm::net::Socket& socket);
    };
}
