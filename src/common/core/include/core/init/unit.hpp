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

    class IEngineUnit {
    public:
        const UnitInfo info;

    private:
        errors::AnyError mSetupResult;
        bool mSetupSuccess;

        virtual void create() throws(AnyException) = 0;
        virtual void destroy() noexcept = 0;

    public:
        IEngineUnit(UnitInfo unitInfo);

        bool setupOk() const noexcept { return mSetupSuccess; }
        const errors::AnyError& result() const noexcept { return mSetupResult; }

        static std::span<IEngineUnit*> all();
    };

    template<typename T>
    class EngineUnit : public IEngineUnit {
        static T gInstance;

    public:
        EngineUnit();
    };
}
