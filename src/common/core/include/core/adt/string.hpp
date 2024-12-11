#pragma once

#include "core/core.hpp"

namespace sm::adt {
    template<typename T>
    class StringBase {
        struct StringData {
            T *mFront;
            T *mBack;
            T *mCapacity;
        };

        struct SmallStorage {
            T mStorage[(sizeof(StringData) / sizeof(T*)) - 1];
            uint8_t mRemaining;
        };

        union InnerData {
            uint8_t mBytes[sizeof(StringData)];
            StringData mString;
            SmallStorage mStorage;
        };

        InnerData mData;

    public:
        constexpr StringBase() noexcept
            : mData(SmallStorage{0, sizeof(SmallStorage::mStorage)})
        { }
    };
}
