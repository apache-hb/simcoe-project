#include "stdafx.hpp" // IWYU pragma: export

#include "game/game.hpp"
#include "std/str.h"

using namespace sm;
using namespace sm::game;
using namespace sm::world;

static constexpr uint kMaxBodies = 1024;
static constexpr uint kBodyMutexCount = 0;
static constexpr uint kMaxBodyPairs = 1024;
static constexpr uint kMaxContactConstraints = 1024;

static game::IContext *gContext = nullptr;

LOG_CATEGORY_IMPL(gPhysics, "physics")
LOG_CATEGORY_IMPL(gGameLog, "game")

game::IContext& game::IContext::get() {
    return *gContext;
}

enum Layers : JPH::ObjectLayer {
    eStatic,
    eDynamic,
    eCount
};

namespace layers {
    static constexpr JPH::BroadPhaseLayer kStatic{ JPH::ObjectLayer(Layers::eStatic) };
    static constexpr JPH::BroadPhaseLayer kDynamic{ JPH::ObjectLayer(Layers::eDynamic) };
    static constexpr uint kCount = uint(Layers::eCount);
}

struct CObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter {
    bool ShouldCollide(JPH::ObjectLayer layer1, JPH::ObjectLayer layer2) const override {
        switch (layer1) {
        case Layers::eStatic:
            return layer2 == Layers::eDynamic;
        case Layers::eDynamic:
            return true;

        default:
            CT_NEVER("Unknown layer");
        }
    }
};

class CBroadPhaseLayer final : public JPH::BroadPhaseLayerInterface {
    JPH::BroadPhaseLayer mLayers[layers::kCount];
public:
    CBroadPhaseLayer() {
        mLayers[uint(Layers::eStatic)] = JPH::BroadPhaseLayer(layers::kStatic);
        mLayers[uint(Layers::eDynamic)] = JPH::BroadPhaseLayer(layers::kDynamic);
    }

    uint GetNumBroadPhaseLayers() const override {
        return layers::kCount;
    }

    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer index) const override {
        return mLayers[index];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char *GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override {
        switch (layer.GetValue()) {
        case Layers::eStatic: return "Static";
        case Layers::eDynamic: return "Dynamic";
        default: return "Unknown";
        }
    }
#endif
};

struct CObjectVsBroadPhaseFilter final : public JPH::ObjectVsBroadPhaseLayerFilter {
    bool ShouldCollide(JPH::ObjectLayer layer, JPH::BroadPhaseLayer bp) const override {
        switch (layer) {
        case Layers::eStatic:
            return bp == layers::kDynamic;
        case Layers::eDynamic:
            return true;

        default:
            CT_NEVER("Unknown layer");
        }
    }
};

struct CContactListener final : public JPH::ContactListener {
    JPH::ValidateResult OnContactValidate(const JPH::Body& body1, const JPH::Body& body2, JPH::RVec3Arg offset, const JPH::CollideShapeResult& collision) override {
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }
};

struct CBodyActivationListener final : public JPH::BodyActivationListener {
    void OnBodyActivated(const JPH::BodyID& id, uint64 user) override {
        gPhysics.info("Body activated: {}", id.GetIndex());
    }

    void OnBodyDeactivated(const JPH::BodyID& id, uint64 user) override {
        gPhysics.info("Body deactivated: {}", id.GetIndex());
    }
};

static char gTraceBuffer[2048];

class Context final : public game::IContext {
    [[maybe_unused]] world::World& mWorld;

    constexpr static float kTimeStep = 1.0f / 60.0f;
    float mTimeAccumulator = 0.0f;

    void tick(float dt) override {
        mTimeAccumulator += dt;

        int steps = int(mTimeAccumulator / kTimeStep);

        if (JPH::EPhysicsUpdateError err = mPhysicsSystem->Update(dt, steps, *mPhysicsAllocator, *mPhysicsThreadPool); err != JPH::EPhysicsUpdateError::None) {
            gPhysics.warn("Physics update error: {}", std::to_underlying(err));
        }
    }

    void shutdown() override {
        shutdown_jph();
    }

    ///
    /// jolt physics
    ///

    static void jph_trace(const char *fmt, ...) { // NOLINT
        va_list args;
        va_start(args, fmt);
        str_sprintf(gTraceBuffer, sizeof(gTraceBuffer), fmt, args);
        va_end(args);

        gPhysics.trace("{}", gTraceBuffer);
    }

    static bool jph_assert(const char *expr, const char *message, const char *file, uint line) {
        gPhysics.panic("Assertion failed: {} in {}:{}\n{}", expr, file, line, message);
        return true;
    }

    sm::UniquePtr<JPH::TempAllocatorImpl> mPhysicsAllocator;
    sm::UniquePtr<JPH::JobSystemThreadPool> mPhysicsThreadPool;
    sm::UniquePtr<JPH::PhysicsSystem> mPhysicsSystem;

    sm::UniquePtr<CBroadPhaseLayer> mBroadPhaseLayer;
    sm::UniquePtr<CObjectLayerPairFilter> mObjectLayerPairFilter;
    sm::UniquePtr<CObjectVsBroadPhaseFilter> mObjectVsBroadPhaseLayerFilter;
    sm::UniquePtr<CContactListener> mContactListener;
    sm::UniquePtr<CBodyActivationListener> mBodyActivationListener;

    void shutdown_jph() {
        JPH::UnregisterTypes();

        delete JPH::Factory::sInstance;
    }

    void init_jph() {
        JPH::RegisterDefaultAllocator();

        JPH::Trace = jph_trace;
        JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = jph_assert;)
        JPH::Factory::sInstance = new JPH::Factory();

        JPH::RegisterTypes();

        mPhysicsAllocator = sm::make_unique<JPH::TempAllocatorImpl>((16_mb).as_bytes());
        mPhysicsThreadPool = sm::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, 4);

        mBroadPhaseLayer = sm::make_unique<CBroadPhaseLayer>();
        mObjectLayerPairFilter = sm::make_unique<CObjectLayerPairFilter>();
        mObjectVsBroadPhaseLayerFilter = sm::make_unique<CObjectVsBroadPhaseFilter>();

        mPhysicsSystem = sm::make_unique<JPH::PhysicsSystem>();
        mPhysicsSystem->Init(kMaxBodies, kBodyMutexCount, kMaxBodyPairs, kMaxContactConstraints, mBroadPhaseLayer.ref(), mObjectVsBroadPhaseLayerFilter.ref(), mObjectLayerPairFilter.ref());

        mBodyActivationListener = sm::make_unique<CBodyActivationListener>();
        mPhysicsSystem->SetBodyActivationListener(*mBodyActivationListener);

        mContactListener = sm::make_unique<CContactListener>();
        mPhysicsSystem->SetContactListener(*mContactListener);
    }

public:
    Context(world::World& world)
        : mWorld(world)
    {
        init_jph();
    }
};

game::IContext& game::init(world::World& world) {
    gContext = new Context(world);

    return IContext::get();
}
