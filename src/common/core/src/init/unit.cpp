#include "core/init/unit.hpp"

using namespace sm;
using namespace sm::init;

static std::vector<IEngineUnit*>& allEngineUnits() {
    static std::vector<IEngineUnit*> sUnits;
    return sUnits;
}

static UnitId addUnit(IEngineUnit *unit) {
    std::vector<IEngineUnit*>& units = allEngineUnits();
    size_t index = units.size();
    units.emplace_back(unit);
    return index;
}

IEngineUnit::IEngineUnit(UnitInfo unitInfo)
    : info(std::move(unitInfo))
    , id(addUnit(this))
    , mState(InitState::ePending)
{ }

bool IEngineUnit::createUnit() try {
    create();
    mState = InitState::eSuccess;
    return true;
} catch (const errors::AnyException& e) {
    mState = InitState::eFailure;
    mSetupStatus = e.cause();
    return false;
}

void IEngineUnit::destroyUnit() noexcept {
    if (mState == InitState::eSuccess) {
        destroy();
    }
}

std::span<IEngineUnit*> IEngineUnit::all() {
    return allEngineUnits();
}
