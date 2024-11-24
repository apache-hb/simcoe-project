#include "core/init/unit.hpp"

using namespace sm;
using namespace sm::init;

static std::vector<IEngineUnit*>& allEngineUnits() {
    static std::vector<IEngineUnit*> sUnits;
    return sUnits;
}

IEngineUnit::IEngineUnit(UnitInfo unitInfo)
    : info(std::move(unitInfo))
    , mSetupSuccess(false)
{
    allEngineUnits().emplace_back(this);
}

std::span<IEngineUnit*> IEngineUnit::all() {
    return allEngineUnits();
}
