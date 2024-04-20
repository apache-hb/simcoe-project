#pragma once

#include "core/small_string.hpp"

#include "core/format.hpp" // IWYU pragma: export

namespace sm {
    struct Memory {
        // dont use reflection enums here, this does quite bespoke to_string behaviour
        enum Unit {
            eBytes,
            eKilobytes,
            eMegabytes,
            eGigabytes,
            eTerabytes,
            eLimit
        };

        static constexpr size_t kByte = 1;
        static constexpr size_t kKilobyte = kByte * 1024;
        static constexpr size_t kMegabyte = kKilobyte * 1024;
        static constexpr size_t kGigabyte = kMegabyte * 1024;
        static constexpr size_t kTerabyte = kGigabyte * 1024;

        static constexpr size_t kSizes[eLimit] = {
            kByte,
            kKilobyte,
            kMegabyte,
            kGigabyte,
            kTerabyte
        };

        static constexpr const char *kNames[eLimit] = {
            "b", "kb", "mb", "gb", "tb"
        };

        constexpr Memory(size_t memory = 0, Unit unit = eBytes)
            : mBytes(memory * kSizes[unit])
        { }

        constexpr size_t b() const { return mBytes; }
        constexpr size_t kb() const { return mBytes / kKilobyte; }
        constexpr size_t mb() const { return mBytes / kMegabyte; }
        constexpr size_t gb() const { return mBytes / kGigabyte; }
        constexpr size_t tb() const { return mBytes / kTerabyte; }

        constexpr size_t as_bytes() const { return mBytes; }
        constexpr size_t as_kilobytes() const { return mBytes / kKilobyte; }
        constexpr size_t as_megabytes() const { return mBytes / kMegabyte; }
        constexpr size_t as_gigabytes() const { return mBytes / kGigabyte; }
        constexpr size_t as_terabytes() const { return mBytes / kTerabyte; }

        friend constexpr auto operator<=>(const Memory& lhs, const Memory& rhs) = default;

        SmallString<64> to_string() const;

    private:
        size_t mBytes;
    };

    constexpr Memory bytes(size_t bytes) { return Memory(bytes, Memory::eBytes); }
    constexpr Memory kilobytes(size_t kilobytes) { return Memory(kilobytes, Memory::eKilobytes); }
    constexpr Memory megabytes(size_t megabytes) { return Memory(megabytes, Memory::eMegabytes); }
    constexpr Memory gigabytes(size_t gigabytes) { return Memory(gigabytes, Memory::eGigabytes); }
    constexpr Memory terabytes(size_t terabytes) { return Memory(terabytes, Memory::eTerabytes); }

    constexpr Memory operator""_b(unsigned long long bytes) { return Memory(bytes, Memory::eBytes); }
    constexpr Memory operator""_kb(unsigned long long kilobytes) { return Memory(kilobytes, Memory::eKilobytes); }
    constexpr Memory operator""_mb(unsigned long long megabytes) { return Memory(megabytes, Memory::eMegabytes); }
    constexpr Memory operator""_gb(unsigned long long gigabytes) { return Memory(gigabytes, Memory::eGigabytes); }
    constexpr Memory operator""_tb(unsigned long long terabytes) { return Memory(terabytes, Memory::eTerabytes); }
}

template<>
struct fmt::formatter<sm::Memory> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const sm::Memory& value, format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", value.to_string().c_str());
    }
};
