#pragma once

#include <stdint.h>

namespace sm::rpc {
    struct Uuid {
        uint64_t low;
        uint64_t high;
    };

    enum class PacketFlags0 : uint8_t {
        eReserved0 = (1 << 0),
        eLastFragment = (1 << 1),
        eMultiFragment = (1 << 2),
        eNotAck = (1 << 3),
        eMaybe = (1 << 4),
        eIdempotent = (1 << 5),
        eBroadcast = (1 << 6),
        eReserved1 = (1 << 7),
    };

    enum class PacketFlags1 : uint8_t {
        eReserved0 = (1 << 0),
        eCancelPending = (1 << 1),
        eReserved1 = (1 << 2),
        eReserved2 = (1 << 3),
        eReserved3 = (1 << 4),
        eReserved4 = (1 << 5),
        eReserved5 = (1 << 6),
        eReserved6 = (1 << 7),
    };

    // cl prefixed types are for connectionless packets
    struct [[gnu::packed]] ClPacketHeader {
        uint8_t version;
        uint8_t ptype;
        PacketFlags0 flags0;
        PacketFlags1 flags1;
        uint8_t drep[3];
        uint8_t serialHi;
        Uuid object;
        Uuid interfaceId;
        Uuid activityId;
        uint32_t interfaceVersion;
        uint32_t sequence;
        uint16_t operation;
        uint16_t interfaceHint;
        uint16_t activityHint;
        uint16_t length;
        uint16_t fragment;
        uint8_t authProtocol;
        uint8_t serialLo;
    };

    // cb prefixed types are for connection based packets
    struct [[gnu::packed]] CbPacketHeader {

    };
}
