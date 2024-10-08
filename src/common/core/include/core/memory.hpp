#pragma once

#include "core/adt/small_string.hpp"

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

            eCount
        };

        static constexpr size_t kByte = 1;
        static constexpr size_t kKilobyte = kByte * 1024;
        static constexpr size_t kMegabyte = kKilobyte * 1024;
        static constexpr size_t kGigabyte = kMegabyte * 1024;
        static constexpr size_t kTerabyte = kGigabyte * 1024;

        using String = SmallString<64>;

        static constexpr size_t kSizes[eCount] = {
            kByte,
            kKilobyte,
            kMegabyte,
            kGigabyte,
            kTerabyte
        };

        static constexpr const char *kNames[eCount] = {
            "b", "kb", "mb", "gb", "tb"
        };

        constexpr Memory(size_t memory = 0, Unit unit = eBytes) noexcept
            : mBytes(memory * kSizes[unit])
        { }

        constexpr size_t asBytes() const noexcept { return mBytes; }
        constexpr size_t asKilobytes() const noexcept { return mBytes / kKilobyte; }
        constexpr size_t asMegabytes() const noexcept { return mBytes / kMegabyte; }
        constexpr size_t asGigabytes() const noexcept { return mBytes / kGigabyte; }
        constexpr size_t asTerabytes() const noexcept { return mBytes / kTerabyte; }

        friend constexpr auto operator<=>(const Memory& lhs, const Memory& rhs) = default;

        String toString() const noexcept;

    private:
        size_t mBytes;
    };

    constexpr Memory bytes(size_t bytes) noexcept { return Memory(bytes, Memory::eBytes); }
    constexpr Memory kilobytes(size_t kilobytes) noexcept { return Memory(kilobytes, Memory::eKilobytes); }
    constexpr Memory megabytes(size_t megabytes) noexcept { return Memory(megabytes, Memory::eMegabytes); }
    constexpr Memory gigabytes(size_t gigabytes) noexcept { return Memory(gigabytes, Memory::eGigabytes); }
    constexpr Memory terabytes(size_t terabytes) noexcept { return Memory(terabytes, Memory::eTerabytes); }

    constexpr Memory operator""_b(unsigned long long bytes) noexcept { return Memory(bytes, Memory::eBytes); }
    constexpr Memory operator""_kb(unsigned long long kilobytes) noexcept { return Memory(kilobytes, Memory::eKilobytes); }
    constexpr Memory operator""_mb(unsigned long long megabytes) noexcept { return Memory(megabytes, Memory::eMegabytes); }
    constexpr Memory operator""_gb(unsigned long long gigabytes) noexcept { return Memory(gigabytes, Memory::eGigabytes); }
    constexpr Memory operator""_tb(unsigned long long terabytes) noexcept { return Memory(terabytes, Memory::eTerabytes); }
}

template<>
struct fmt::formatter<sm::Memory> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    constexpr auto format(const sm::Memory& value, format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", value.toString().c_str());
    }
};
