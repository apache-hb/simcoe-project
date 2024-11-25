#pragma once

#include "core/error/error.hpp"

#include "core/version_def.h"

#include <span>
#include <string>

namespace sm::init {
    struct UnitInfo {
        std::string name;
        std::string description;
        std::string author;
        std::string license;
        ctu_version_t version;
    };

    using UnitId = size_t;

    enum InitState {
        ePending,
        eSuccess,
        eFailure,
    };

    class IEngineUnit {
    public:
        const UnitInfo info;
        const UnitId id;

    private:
        errors::AnyError mSetupStatus;
        InitState mState;

        virtual void create() throws(AnyException) = 0;
        virtual void destroy() noexcept = 0;

    public:
        IEngineUnit(UnitInfo unitInfo);

        InitState state() const noexcept { return mState; }
        const errors::AnyError& status() const noexcept { return mSetupStatus; }

        bool createUnit();
        void destroyUnit() noexcept;

        static std::span<IEngineUnit*> all();
    };

    template<typename T>
    class EngineUnit : public IEngineUnit {
        static T gInstance;

    public:
        EngineUnit(UnitInfo info);
    };

    class UnitCollection {

    };
}
