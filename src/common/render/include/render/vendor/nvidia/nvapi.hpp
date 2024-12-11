#pragma once

#include "core/adt/small_string.hpp"

#include <expected>

namespace sm::render {
    using NvString = SmallString<64>;

    class NvStatus {
        int mStatus;

    public:
        constexpr NvStatus(int status = 0) noexcept
            : mStatus(status)
        { }

        constexpr bool success() const noexcept { return mStatus == 0; }

        NvString message() const;
    };

    template<typename T>
    using NvResult = std::expected<T, NvStatus>;

    struct NvVersion {
        uint32_t version;
        NvString build;
    };

    namespace nvapi {
        NvStatus create(void);
        NvStatus destroy(void) noexcept;

        NvResult<NvString> getInterfaceVersionString();
        NvResult<NvString> getInterfaceVersionStringEx();
        NvResult<NvVersion> getDriverVersion();
    }
}
