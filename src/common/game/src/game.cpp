#include "stdafx.hpp" // IWYU pragma: export

#include "math/colour.hpp"

#include "game/game.hpp"
#include "world/ecs.hpp"
#include "std/str.h"
#include "math/format.hpp"

using namespace sm;
using namespace sm::game;
using namespace sm::world;

using namespace sm::math::literals;
using namespace JPH::literals;

static constexpr uint kMaxBodies = 1024;
static constexpr uint kBodyMutexCount = 0;
static constexpr uint kMaxBodyPairs = 1024;
static constexpr uint kMaxContactConstraints = 1024;
static const uint kPhysicsThreads = std::thread::hardware_concurrency() - 1;

static game::GameContextImpl *gContext = nullptr;
static constexpr float kTimeStep = 1.0f / 60.0f;

game::Context game::getContext() {
    return gContext;
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

static JPH::Vec3 to_jph(const math::float3& v) {
    return {v.x, v.y, v.z};
}

static math::float3 from_jph(const JPH::Vec3& v) {
    return {v.GetX(), v.GetY(), v.GetZ()};
}

static JPH::Quat to_jph(const math::quatf& q) {
    return {q.v.x, q.v.y, q.v.z, q.w};
}

static math::quatf from_jph(const JPH::Quat& q) {
    return math::quatf{q.GetX(), q.GetY(), q.GetZ(), q.GetW()};
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
    void OnBodyActivated(const JPH::BodyID& id, uint64_t user) override {
        LOG_INFO(PhysicsLog, "Body activated: {}", id.GetIndex());
    }

    void OnBodyDeactivated(const JPH::BodyID& id, uint64_t user) override {
        LOG_INFO(PhysicsLog, "Body deactivated: {}", id.GetIndex());
    }
};

struct DebugVertex {
    math::float3 position;
    math::float3 colour;
};

#if SMC_DEBUG
struct CDebugRenderer final : public JPH::DebugRendererSimple {
    using Super = JPH::DebugRendererSimple;

    sm::VectorBase<DebugVertex> mLines;

    sm::VectorBase<DebugVertex> mTriangles;

    void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override {
        math::float3 from = from_jph(inFrom);
        math::float3 to = from_jph(inTo);
        math::float3 colour = math::unpack_colour(inColor.GetUInt32()).xyz();

        mLines.push_back({ from, colour });
        mLines.push_back({ to, colour });

        // gPhysicsLog.info("DrawLine: {} -> {}", from, to);
    }

    void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) override {
        math::float3 v1 = from_jph(inV1);
        math::float3 v2 = from_jph(inV2);
        math::float3 v3 = from_jph(inV3);
        math::float3 colour = math::unpack_colour(inColor.GetUInt32()).xyz();

        mTriangles.push_back({ v1, colour });
        mTriangles.push_back({ v2, colour });
        mTriangles.push_back({ v3, colour });

        // gPhysicsLog.info("DrawTriangle: {} -> {} -> {}", v1, v2, v3);
    }

    void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) override {
        LOG_INFO(PhysicsLog, "DrawText3D: {}", inString);
    }

    void begin_frame(flecs::entity camera) {
        mLines.clear();
        mTriangles.clear();

        const world::ecs::Position *position = camera.get<world::ecs::Position, world::ecs::World>();

        Super::SetCameraPos(to_jph(position->position));
    }
};
#endif

static void jph_trace(const char *fmt, ...) { // NOLINT
    char buffer[2048];
    va_list args;
    va_start(args, fmt);
    str_sprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    LOG_TRACE(PhysicsLog, "{}", buffer);
}

// constexpr static float kTimeStep = 1.0f / 60.0f;

struct game::PhysicsBodyImpl {
    JPH::Body *body;
    JPH::BodyID id;
    GameContextImpl *context;
};

struct game::CharacterBodyImpl {
    // JPH::CharacterVirtual *vbody;
    JPH::Character *body;
    JPH::BodyID id;
    GameContextImpl *context;
};

struct game::GameContextImpl {
    ///
    /// timestep
    ///
    float mTimeAccumulator = 0.0f;

    // maximum number of steps per tick
    // after this we split the ticks into smaller steps.
    // this avoids resource exhaustion inside jolt and the
    // subsequent assertion failures.
    int mMaxStepsPerTick = 5;

    ///
    /// misc
    ///

    world::World& world;
    flecs::entity activeCamera;

    ///
    /// gameplay
    ///

    // sm::HashMap<uint32, game::Archetype> archetypes;

    // sm::Vector<game::Object*> objects;

    ///
    /// jolt physics
    ///
    sm::UniquePtr<JPH::TempAllocatorImpl> physicsAllocator;
    sm::UniquePtr<JPH::JobSystemSingleThreaded> physicsThreadPool;
    sm::UniquePtr<JPH::PhysicsSystem> physicsSystem;

    sm::UniquePtr<CBroadPhaseLayer> broadPhaseLayer;
    sm::UniquePtr<CObjectLayerPairFilter> objectLayerPairFilter;
    sm::UniquePtr<CObjectVsBroadPhaseFilter> objectVsBroadPhaseLayerFilter;
    sm::UniquePtr<CContactListener> contactListener;
    sm::UniquePtr<CBodyActivationListener> bodyActivationListener;

#if SMC_DEBUG
    sm::UniquePtr<CDebugRenderer> debugRenderer;
#endif

    void init() {
        JPH::RegisterDefaultAllocator();

        JPH::Trace = jph_trace;

        JPH_IF_ENABLE_ASSERTS({
            JPH::AssertFailed = [](const char *expr, const char *message, const char *file, uint line) -> bool {
                if (message == nullptr)
                    gPhysicsLog.panic("Assertion failed: {} in {}:{}\n", expr, file, line);
                else
                    gPhysicsLog.panic("Assertion failed: {} in {}:{}\n{}", expr, file, line, message);

                return true;
            };
        })

        JPH::Factory::sInstance = new JPH::Factory();

#if SMC_DEBUG
        debugRenderer = sm::makeUnique<CDebugRenderer>();
        JPH::DebugRenderer::sInstance = debugRenderer.get();
#endif

        JPH::RegisterTypes();

        physicsAllocator = sm::makeUnique<JPH::TempAllocatorImpl>((16_mb).asBytes());
        physicsThreadPool = sm::makeUnique<JPH::JobSystemSingleThreaded>(0x1000 * 4);

        broadPhaseLayer = sm::makeUnique<CBroadPhaseLayer>();
        objectLayerPairFilter = sm::makeUnique<CObjectLayerPairFilter>();
        objectVsBroadPhaseLayerFilter = sm::makeUnique<CObjectVsBroadPhaseFilter>();

        JPH::PhysicsSettings settings = {
            .mTimeBeforeSleep = 5.f,
        };

        physicsSystem = sm::makeUnique<JPH::PhysicsSystem>();
        physicsSystem->SetPhysicsSettings(settings);
        physicsSystem->Init(kMaxBodies, kBodyMutexCount, kMaxBodyPairs, kMaxContactConstraints, broadPhaseLayer.ref(), objectVsBroadPhaseLayerFilter.ref(), objectLayerPairFilter.ref());

        bodyActivationListener = sm::makeUnique<CBodyActivationListener>();
        physicsSystem->SetBodyActivationListener(*bodyActivationListener);

        contactListener = sm::makeUnique<CContactListener>();
        physicsSystem->SetContactListener(*contactListener);

#if SMC_DEBUG
        debugRenderer->DrawCoordinateSystem(JPH::RMat44::sIdentity());
#endif
    }
};

game::Context game::init(world::World& world, flecs::entity camera) {
    CTASSERTF(gContext == nullptr, "Context already initialized");

    gContext = new GameContextImpl{.world = world};

    Context ctx = getContext();

    ctx.setCamera(camera);

    gContext->init();

    return ctx;
}

void game::cbiDestroy(CharacterBodyImpl *body) {
    JPH::BodyInterface& factory = body->context->physicsSystem->GetBodyInterface();
    factory.RemoveBody(body->id);
    factory.DestroyBody(body->id);
    delete body;
}

void game::pbiDestroy(PhysicsBodyImpl *body) {
    JPH::BodyInterface& factory = body->context->physicsSystem->GetBodyInterface();
    factory.RemoveBody(body->id);
    factory.DestroyBody(body->id);
    delete body;
}

void CharacterBody::update(float dt) {
#if 0
    GameContextImpl *ctx = mImpl->context;

    JPH::Vec3 up = mImpl->vbody->GetUp();

    JPH::CharacterVirtual::ExtendedUpdateSettings settings;
    settings.mStickToFloorStepDown = -up * settings.mStickToFloorStepDown.Length();
    settings.mWalkStairsStepUp = up * settings.mWalkStairsStepUp.Length();

    mImpl->vbody->ExtendedUpdate(
        /*inDeltaTime=*/ dt,
        /*inGravity=*/ (-up * ctx->physicsSystem->GetGravity().Length()),
        /*inSettings=*/ settings,
        /*inBroadPhaseLayerFilter=*/ ctx->physicsSystem->GetDefaultBroadPhaseLayerFilter(Layers::eDynamic),
        /*inObjectLayerFilter=*/ ctx->physicsSystem->GetDefaultLayerFilter(Layers::eDynamic),
        /*inBodyFilter=*/ { },
        /*inShapeFilter=*/ { },
        /*inAllocator=*/ ctx->physicsAllocator.ref()
    );
#endif
}

math::float3 CharacterBody::getUpVector() const {
    return from_jph(mImpl->body->GetUp());
}

void CharacterBody::setUpVector(const math::float3& up) {
    mImpl->body->SetUp(to_jph(up));
}

math::float3 CharacterBody::getPosition() const {
    return from_jph(mImpl->body->GetPosition());
}

math::quatf CharacterBody::getRotation() const {
    return from_jph(mImpl->body->GetRotation());
}

void CharacterBody::setRotation(const math::quatf& rotation) {
    mImpl->body->SetRotation(to_jph(rotation));
}

math::float3 CharacterBody::getGroundNormal() const {
    return from_jph(mImpl->body->GetGroundNormal());
}

math::float3 CharacterBody::getGroundVelocity() const {
    return from_jph(mImpl->body->GetGroundVelocity());
}

void CharacterBody::updateGroundVelocity() {
    // mImpl->body->UpdateGroundVelocity();
}

math::float3 CharacterBody::getLinearVelocity() const {
    return from_jph(mImpl->body->GetLinearVelocity());
}

void CharacterBody::setLinearVelocity(const math::float3& velocity) {
    // mImpl->body->SetLinearVelocity(to_jph(velocity), true);
    mImpl->body->SetLinearVelocity(to_jph(velocity));
}

bool CharacterBody::isOnGround() const {
    return mImpl->body->GetGroundState() == JPH::Character::EGroundState::OnGround;
}

bool CharacterBody::isOnSteepSlope() const {
    return mImpl->body->GetGroundState() == JPH::Character::EGroundState::OnSteepGround;
}

bool CharacterBody::isInAir() const {
    return mImpl->body->GetGroundState() == JPH::Character::EGroundState::InAir;
}

bool CharacterBody::isNotSupported() const {
    return mImpl->body->GetGroundState() == JPH::Character::EGroundState::NotSupported;
}

bool CharacterBody::isSupported() const {
    return mImpl->body->IsSupported();
}

void CharacterBody::postUpdate() {
    mImpl->body->PostSimulation(0.05f);
}

void game::Context::debugDrawPhysicsBody(const PhysicsBody& body) {
#if SMC_DEBUG
    const JPH::Body *object = body.getImpl()->body;
    const JPH::Shape *shape = object->GetShape();

    shape->Draw(*mImpl->debugRenderer, object->GetCenterOfMassTransform(), JPH::Vec3::sReplicate(1.0f), JPH::Color::sGreen, false, false);
#endif
}

// void game::Context::addClass(const meta::Class& cls) {
// }

// void game::Context::addObject(game::Object *object) {
//     mImpl->objects.push_back(object);
//     object->create();
// }

// void game::Context::destroyObject(game::Object *object) {
//     object->destroy();
// }

void game::Context::tick(float dt) {
#if SMC_DEBUG
    mImpl->debugRenderer->begin_frame(mImpl->activeCamera);
#endif

    int steps = 1;
    if (dt > kTimeStep) {
        steps = std::max(1, int(ceilf(dt / kTimeStep)));
    }

    if (steps > mImpl->mMaxStepsPerTick) {
        LOG_WARN(PhysicsLog, "Abnormal amount of physics steps: {} (delta: {})", steps, dt);
    }

    while (steps > mImpl->mMaxStepsPerTick) {
        if (JPH::EPhysicsUpdateError err = mImpl->physicsSystem->Update(dt, mImpl->mMaxStepsPerTick, *mImpl->physicsAllocator, *mImpl->physicsThreadPool); err != JPH::EPhysicsUpdateError::None) {
            LOG_WARN(PhysicsLog, "Physics update error: {}", std::to_underlying(err));
        }

        steps -= mImpl->mMaxStepsPerTick;
    }

    if (steps != 0) {
        if (JPH::EPhysicsUpdateError err = mImpl->physicsSystem->Update(dt, steps, *mImpl->physicsAllocator, *mImpl->physicsThreadPool); err != JPH::EPhysicsUpdateError::None) {
            LOG_WARN(PhysicsLog, "Physics update error: {}", std::to_underlying(err));
        }
    }

    // for (game::Object *object : mImpl->objects) {
    //     object->update(dt);
    // }
}

math::float3 game::Context::getGravity() const {
    return from_jph(mImpl->physicsSystem->GetGravity());
}

void game::Context::shutdown() {
    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
}

void game::Context::setCamera(flecs::entity camera) {
    mImpl->activeCamera = camera;
}

PhysicsBody game::Context::addPhysicsBody(const world::Cube& shape, const math::float3& position, const math::quatf& rotation, bool dynamic) {
    JPH::BodyInterface& factory = mImpl->physicsSystem->GetBodyInterface();

    JPH::BoxShapeSettings box{JPH::Vec3(shape.width, shape.height, shape.depth)};
    JPH::ShapeSettings::ShapeResult result = box.Create();

    JPH::EMotionType motion = dynamic ? JPH::EMotionType::Dynamic : JPH::EMotionType::Static;
    JPH::ObjectLayer layer = dynamic ? Layers::eDynamic : Layers::eStatic;
    JPH::BodyCreationSettings settings{
        result.Get(), to_jph(position), to_jph(rotation), motion, layer
    };

    JPH::Body *body = factory.CreateBody(settings);

    factory.AddBody(body->GetID(), JPH::EActivation::DontActivate);

    return new PhysicsBodyImpl(body, body->GetID(), mImpl);
}

PhysicsBody game::Context::addPhysicsBody(const world::Sphere& shape, const math::float3& position, const math::quatf& rotation, bool dynamic) {
    JPH::BodyInterface& factory = mImpl->physicsSystem->GetBodyInterface();

    JPH::SphereShapeSettings sphere{shape.radius};
    JPH::ShapeSettings::ShapeResult result = sphere.Create();

    JPH::EMotionType motion = dynamic ? JPH::EMotionType::Dynamic : JPH::EMotionType::Static;
    JPH::ObjectLayer layer = dynamic ? Layers::eDynamic : Layers::eStatic;
    JPH::BodyCreationSettings settings{
        result.Get(), to_jph(position), to_jph(rotation), motion, layer
    };

    JPH::Body *body = factory.CreateBody(settings);

    factory.AddBody(body->GetID(), JPH::EActivation::DontActivate);

    return new PhysicsBodyImpl(body, body->GetID(), mImpl);
}

CharacterBody game::Context::addCharacterBody(
    const world::Cylinder& shape,
    const math::float3& position,
    const math::quatf& rotation,
    bool activiate)
{
    JPH::Ref physicsShape = new JPH::CylinderShape(shape.height / 2.f, shape.radius);

#if 0
    JPH::Ref vbodySettings = new JPH::CharacterVirtualSettings();
    vbodySettings->mMaxSlopeAngle = (45._deg).get_radians();
    vbodySettings->mUp = JPH::Vec3::sAxisZ();
    vbodySettings->mShape = (new JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * shape.height, 0), JPH::Quat::sIdentity(), physicsShape))->Create().Get();
    vbodySettings->mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -shape.radius);

    JPH::CharacterVirtual *vbody = new JPH::CharacterVirtual(vbodySettings, to_jph(position), to_jph(rotation), 0, *mImpl->physicsSystem);
#endif

    JPH::Ref bodySettings = new JPH::CharacterSettings();
    bodySettings->mMaxSlopeAngle = (45._deg).get_radians();
    bodySettings->mLayer = Layers::eDynamic;
    bodySettings->mShape = (new JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * shape.height, 0), JPH::Quat::sIdentity(), physicsShape))->Create().Get();
    bodySettings->mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -shape.radius);

    JPH::Character *body = new JPH::Character(bodySettings, to_jph(position), to_jph(rotation), 0, *mImpl->physicsSystem);

    body->AddToPhysicsSystem(activiate ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);

    return new CharacterBodyImpl{body, body->GetBodyID(), mImpl};
}

world::World& game::Context::getWorld() {
    return mImpl->world;
}

void PhysicsBody::setLinearVelocity(const math::float3& velocity) {
    JPH::BodyInterface& factory = mImpl->context->physicsSystem->GetBodyInterface();
    factory.SetLinearVelocity(mImpl->id, to_jph(velocity));
}

void PhysicsBody::setAngularVelocity(const math::float3& velocity) {
    JPH::BodyInterface& factory = mImpl->context->physicsSystem->GetBodyInterface();
    factory.SetAngularVelocity(mImpl->id, to_jph(velocity));
}

void PhysicsBody::activate() {
    JPH::BodyInterface& factory = mImpl->context->physicsSystem->GetBodyInterface();
    factory.ActivateBody(mImpl->id);
}

bool PhysicsBody::isActive() const {
    JPH::BodyInterface& factory = mImpl->context->physicsSystem->GetBodyInterface();
    return factory.IsActive(mImpl->id);
}

math::float3 PhysicsBody::getLinearVelocity() const {
    JPH::BodyInterface& factory = mImpl->context->physicsSystem->GetBodyInterface();
    return from_jph(factory.GetLinearVelocity(mImpl->id));
}

math::float3 PhysicsBody::getCenterOfMass() const {
    JPH::BodyInterface& factory = mImpl->context->physicsSystem->GetBodyInterface();
    return from_jph(factory.GetCenterOfMassPosition(mImpl->id));
}

math::quatf PhysicsBody::getRotation() const {
    JPH::BodyInterface& factory = mImpl->context->physicsSystem->GetBodyInterface();
    return from_jph(factory.GetRotation(mImpl->id));
}

static const D3D12_ROOT_SIGNATURE_FLAGS kPrimitiveRootFlags
    = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

[[maybe_unused]]
static void createDebugLinePSO(render::IDeviceContext& context, render::Pipeline& pipeline, DXGI_FORMAT colour, DXGI_FORMAT depth) {
    {
        // mvp matrix
        CD3DX12_ROOT_PARAMETER1 params[1];
        params[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
        desc.Init_1_1(1, params, 0, nullptr, kPrimitiveRootFlags);

        context.serialize_root_signature(pipeline.signature, desc);
    }

    {
        auto ps = context.mConfig.bundle->get_shader_bytecode("jolt_debug.ps");
        auto vs = context.mConfig.bundle->get_shader_bytecode("jolt_debug.vs");

        constexpr D3D12_INPUT_ELEMENT_DESC kInputElements[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DebugVertex, position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOUR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DebugVertex, colour), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {
            .pRootSignature = pipeline.signature.get(),
            .VS = CD3DX12_SHADER_BYTECODE(vs.data(), vs.size()),
            .PS = CD3DX12_SHADER_BYTECODE(ps.data(), ps.size()),
            .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
            .SampleMask = UINT_MAX,
            .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
            .DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
            .InputLayout = { kInputElements, _countof(kInputElements) },
            .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,
            .NumRenderTargets = 1,
            .RTVFormats = { colour },
            .DSVFormat = depth,
            .SampleDesc = { 1, 0 },
        };

        desc.DepthStencilState.DepthEnable = FALSE;

        auto device = context.getDevice();

        SM_ASSERT_HR(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipeline.pso)));
    }
}

[[maybe_unused]]
static void createDebugTrianglePSO(
    render::IDeviceContext& context,
    render::Pipeline& pipeline,
    DXGI_FORMAT colour,
    DXGI_FORMAT depth)
{
    {
        // mvp matrix
        CD3DX12_ROOT_PARAMETER1 params[1];
        params[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
        desc.Init_1_1(1, params, 0, nullptr, kPrimitiveRootFlags);

        context.serialize_root_signature(pipeline.signature, desc);
    }

    {
        auto ps = context.mConfig.bundle->get_shader_bytecode("jolt_debug.ps");
        auto vs = context.mConfig.bundle->get_shader_bytecode("jolt_debug.vs");

        constexpr D3D12_INPUT_ELEMENT_DESC kInputElements[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DebugVertex, position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOUR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DebugVertex, colour), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {
            .pRootSignature = pipeline.signature.get(),
            .VS = CD3DX12_SHADER_BYTECODE(vs.data(), vs.size()),
            .PS = CD3DX12_SHADER_BYTECODE(ps.data(), ps.size()),
            .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
            .SampleMask = UINT_MAX,
            .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
            .DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
            .InputLayout = { kInputElements, _countof(kInputElements) },
            .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            .NumRenderTargets = 1,
            .RTVFormats = { colour },
            .DSVFormat = depth,
            .SampleDesc = { 1, 0 },
        };

        desc.DepthStencilState.DepthEnable = FALSE;

        auto device = context.getDevice();

        SM_ASSERT_HR(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipeline.pso)));
    }
}

void game::physics_debug(
    graph::FrameGraph& graph,
    flecs::entity camera,
    graph::Handle target)
{
#if SMC_DEBUG
    const world::ecs::Camera *config = camera.get<world::ecs::Camera>();

    graph::PassBuilder pass = graph.graphics("Physics Debug");
    pass.write(target, "Target", graph::Usage::eRenderTarget);

    graph::ResourceInfo info = graph::ResourceInfo::tex2d(config->window, config->depth)
        .clearDepthStencil(1.f, 0);

    graph::Handle depth = pass.create(info, "Depth", graph::Usage::eDepthWrite);

    auto& data = graph.newDeviceData([config](render::IDeviceContext& context) {
        struct {
            render::Pipeline lines;
            render::Pipeline triangles;

            render::VertexUploadBuffer<DebugVertex> lineBuffer;
            render::VertexUploadBuffer<DebugVertex> triangleBuffer;
        } info;

        createDebugLinePSO(context, info.lines, config->colour, config->depth);
        createDebugTrianglePSO(context, info.triangles, config->colour, config->depth);

        info.lineBuffer = render::newVertexUploadBuffer<DebugVertex>(context, 0x1000uz * 8);
        info.triangleBuffer = render::newVertexUploadBuffer<DebugVertex>(context, 0x1000uz * 8);

        return info;
    });

    pass.bind([target, depth, &data, camera](graph::RenderContext& ctx) {
        auto& context = ctx.context;
        auto *cmd = ctx.commands;

        auto dsv = ctx.graph.dsv(depth);
        auto dsv_cpu = context.mDsvPool.cpu_handle(dsv);

        auto rtv = ctx.graph.rtv(target);
        auto rtv_cpu = context.mRtvPool.cpu_handle(rtv);

        const world::ecs::Camera *config = camera.get<world::ecs::Camera>();

        render::Viewport vp{config->window};
        math::float4x4 m = math::float4x4::identity();
        math::float4x4 v = world::ecs::getViewMatrix(*camera.get<world::ecs::Position, world::ecs::World>(), *camera.get<world::ecs::Direction>());
        math::float4x4 p = config->getProjectionMatrix();
        math::float4x4 mvp = m * v * p;

        auto& debug = static_cast<CDebugRenderer&>(*JPH::DebugRenderer::sInstance);
        if (!debug.mLines.empty()) {
            cmd->SetGraphicsRootSignature(*data.lines.signature);
            cmd->SetPipelineState(*data.lines.pso);

            cmd->RSSetViewports(1, vp.viewport());
            cmd->RSSetScissorRects(1, vp.scissor());

            cmd->OMSetRenderTargets(1, &rtv_cpu, false, &dsv_cpu);

            cmd->SetGraphicsRoot32BitConstants(0, 16, mvp.data(), 0);

            cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

            data.lineBuffer.update(debug.mLines);

            D3D12_VERTEX_BUFFER_VIEW vbv = data.lineBuffer.getView();

            cmd->IASetVertexBuffers(0, 1, &vbv);

            cmd->DrawInstanced(debug.mLines.size(), 1, 0, 0);
        }

        if (!debug.mTriangles.empty()) {
            cmd->SetGraphicsRootSignature(*data.triangles.signature);
            cmd->SetPipelineState(*data.triangles.pso);

            cmd->RSSetViewports(1, vp.viewport());
            cmd->RSSetScissorRects(1, vp.scissor());

            cmd->OMSetRenderTargets(1, &rtv_cpu, false, &dsv_cpu);

            cmd->SetGraphicsRoot32BitConstants(0, 16, mvp.data(), 0);

            cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            data.triangleBuffer.update(debug.mTriangles);

            D3D12_VERTEX_BUFFER_VIEW vbv = data.triangleBuffer.getView();

            cmd->IASetVertexBuffers(0, 1, &vbv);

            cmd->DrawInstanced(debug.mTriangles.size(), 1, 0, 0);
        }
    });
#endif
}