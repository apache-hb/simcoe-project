#pragma once

#include "core/small_string.hpp"

#include <expected>

namespace sm::render {
    using AgsString = SmallString<64>;

    class AgsError {
        int mStatus;

    public:
        AgsError() = default;

        constexpr AgsError(int status)
            : mStatus(status)
        { }

        constexpr bool success() const { return mStatus == 0; }

        AgsString message() const;
    };

    template<typename T>
    using AgsResult = std::expected<T, AgsError>;

    struct AgsVersion {
        int major;
        int minor;
        int patch;
    };

    namespace ags {
        AgsError startup();
        AgsError shutdown();
        AgsVersion getVersion();
    }
}
