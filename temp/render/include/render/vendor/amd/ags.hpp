#pragma once

#include "core/adt/small_string.hpp"

#include <expected>

namespace sm::render {
    using AgsString = SmallString<64>;

    class AgsError {
        int mStatus;

    public:
        constexpr AgsError(int status = 0) noexcept
            : mStatus(status)
        { }

        constexpr bool success() const noexcept { return mStatus == 0; }

        AgsString message() const;
    };

    template<typename T>
    using AgsResult = std::expected<T, AgsError>;

    struct AgsVersion {
        std::string_view driver;
        std::string_view runtime;
        int major;
        int minor;
        int patch;
    };

    namespace ags {
        AgsError create(void);
        AgsError destroy(void) noexcept;
        AgsVersion getVersion();
    }
}
