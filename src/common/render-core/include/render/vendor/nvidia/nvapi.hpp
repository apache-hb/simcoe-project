#pragma once

#include "core/adt/small_string.hpp"
#include <expected>

namespace sm::render {
    using NvString = SmallString<64>;

    class NvStatus {
        int mStatus;

    public:
        NvStatus() = default;

        constexpr NvStatus(int status)
            : mStatus(status)
        { }

        constexpr bool success() const { return mStatus == 0; }

        NvString message() const;
    };

    template<typename T>
    struct NvResult {
        NvStatus status;
        T value;

        bool success() const { return status.success(); }
    };

    struct NvVersion {
        uint32 version;
        NvString build;
    };

    namespace nvapi {
        NvStatus startup();
        NvResult<NvString> getInterfaceVersionString();
        NvResult<NvString> getInterfaceVersionStringEx();
        NvResult<NvVersion> getDriverVersion();
        NvStatus shutdown();
    }
}
