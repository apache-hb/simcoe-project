#pragma once

#include "core/adt/small_string.hpp"

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
    class NvResult {
        NvStatus mStatus;
        T mValue;

    public:
        NvResult(NvStatus status, T value)
            : mStatus(status)
            , mValue(value)
        { }

        constexpr bool success() const { return mStatus.success(); }
        NvStatus status() const { return mStatus; }

        T& value() { return mValue; }
        const T& value() const { return mValue; }
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
