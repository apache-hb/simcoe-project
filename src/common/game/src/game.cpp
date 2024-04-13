#include "stdafx.hpp" // IWYU pragma: export

#include "math/colour.hpp"

#include "game/game.hpp"
#include "std/str.h"

using namespace sm;
using namespace sm::game;
using namespace sm::world;

using namespace JPH::literals;

static constexpr uint kMaxBodies = 1024;
static constexpr uint kBodyMutexCount = 0;
static constexpr uint kMaxBodyPairs = 1024;
static constexpr uint kMaxContactConstraints = 1024;

static game::GameContextImpl *gContext = nullptr;

LOG_CATEGORY_IMPL(gPhysicsLog, "physics")
LOG_CATEGORY_IMPL(gGameLog, "game")

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
    return {v.x, v.z, v.y};
}

static math::float3 from_jph(const JPH::Vec3& v) {
    return {v.GetX(), v.GetZ(), v.GetY()};
}

static JPH::Quat to_jph(const math::quatf& q) {
    math::quatf tmp = q.rotated(math::quatf::from_axis_angle(world::kVectorRight, 90._deg));
    return {tmp.v.x, tmp.v.y, tmp.v.z, tmp.angle};
}

static math::quatf from_jph(const JPH::Quat& q) {
    math::quatf tmp = {q.GetX(), q.GetY(), q.GetZ(), q.GetW()};
    return tmp.rotated(math::quatf::from_axis_angle(world::kVectorRight, -90._deg));
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
        gPhysicsLog.info("Body activated: {}", id.GetIndex());
    }

    void OnBodyDeactivated(const JPH::BodyID& id, uint64 user) override {
        gPhysicsLog.info("Body deactivated: {}", id.GetIndex());
    }
};

struct DebugVertex {
    math::float3 position;
    math::float3 colour;
};

struct CDebugRenderer final : public JPH::DebugRendererSimple {
    using Super = JPH::DebugRendererSimple;

    sm::Vector<DebugVertex> mVertices;

    void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override {
        float3 from = { inFrom.GetX(), inFrom.GetZ(), inFrom.GetY() };
        float3 to = { inTo.GetX(), inTo.GetZ(), inTo.GetY() };
        float3 colour = math::unpack_colour(inColor.GetUInt32()).xyz();

        mVertices.push_back({ from, colour });
        mVertices.push_back({ to, colour });
    }

    void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) override {
        gPhysicsLog.info("DrawText3D: {}", inString);
    }

    void begin_frame(const draw::Camera& camera) {
        mVertices.clear();

        Super::SetCameraPos(to_jph(camera.position()));
    }
};

static char gTraceBuffer[2048];

static void jph_trace(const char *fmt, ...) { // NOLINT
    va_list args;
    va_start(args, fmt);
    str_sprintf(gTraceBuffer, sizeof(gTraceBuffer), fmt, args);
    va_end(args);

    gPhysicsLog.trace("{}", gTraceBuffer);
}

static bool jph_assert(const char *expr, const char *message, const char *file, uint line) {
    if (message == nullptr)
        gPhysicsLog.panic("Assertion failed: {} in {}:{}\n", expr, file, line);
    else
        gPhysicsLog.panic("Assertion failed: {} in {}:{}\n{}", expr, file, line, message);

    return true;
}

// constexpr static float kTimeStep = 1.0f / 60.0f;

struct Context;

struct game::PhysicsBodyImpl {
    JPH::Body *body;
    JPH::BodyID id;
    GameContextImpl *context;
};

struct game::GameContextImpl {
    ///
    /// timestep
    ///
    float mTimeAccumulator = 0.0f;

    ///
    /// misc
    ///

    world::World& world;
    const draw::Camera *activeCamera = nullptr;

    ///
    /// gameplay
    ///

    sm::HashMap<uint32, game::Archetype> archetypes;

    sm::Vector<game::Object*> objects;

    ///
    /// jolt physics
    ///
    sm::UniquePtr<JPH::TempAllocatorImpl> physicsAllocator;
    sm::UniquePtr<JPH::JobSystemThreadPool> physicsThreadPool;
    sm::UniquePtr<JPH::PhysicsSystem> physicsSystem;

    sm::UniquePtr<CBroadPhaseLayer> broadPhaseLayer;
    sm::UniquePtr<CObjectLayerPairFilter> objectLayerPairFilter;
    sm::UniquePtr<CObjectVsBroadPhaseFilter> objectVsBroadPhaseLayerFilter;
    sm::UniquePtr<CContactListener> contactListener;
    sm::UniquePtr<CBodyActivationListener> bodyActivationListener;

    sm::UniquePtr<CDebugRenderer> debugRenderer;
};

static void initGameContext(GameContextImpl *impl) {
    JPH::RegisterDefaultAllocator();

    JPH::Trace = jph_trace;
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = jph_assert;)
    JPH::Factory::sInstance = new JPH::Factory();

    impl->debugRenderer = sm::make_unique<CDebugRenderer>();
    JPH::DebugRenderer::sInstance = impl->debugRenderer.get();

    JPH::RegisterTypes();

    impl->physicsAllocator = sm::make_unique<JPH::TempAllocatorImpl>((16_mb).as_bytes());
    impl->physicsThreadPool = sm::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, 4);

    impl->broadPhaseLayer = sm::make_unique<CBroadPhaseLayer>();
    impl->objectLayerPairFilter = sm::make_unique<CObjectLayerPairFilter>();
    impl->objectVsBroadPhaseLayerFilter = sm::make_unique<CObjectVsBroadPhaseFilter>();

    impl->physicsSystem = sm::make_unique<JPH::PhysicsSystem>();
    impl->physicsSystem->Init(kMaxBodies, kBodyMutexCount, kMaxBodyPairs, kMaxContactConstraints, impl->broadPhaseLayer.ref(), impl->objectVsBroadPhaseLayerFilter.ref(), impl->objectLayerPairFilter.ref());

    impl->bodyActivationListener = sm::make_unique<CBodyActivationListener>();
    impl->physicsSystem->SetBodyActivationListener(*impl->bodyActivationListener);

    impl->contactListener = sm::make_unique<CContactListener>();
    impl->physicsSystem->SetContactListener(*impl->contactListener);

    impl->debugRenderer->DrawCoordinateSystem(JPH::RMat44::sIdentity());
}

game::Context game::init(world::World& world, const draw::Camera& camera) {
    CTASSERTF(gContext == nullptr, "Context already initialized");

    gContext = new GameContextImpl{.world = world};

    initGameContext(gContext);

    Context ctx = getContext();
    ctx.setCamera(camera);

    return ctx;
}

void game::pbiDestroy(PhysicsBodyImpl *body) {
    JPH::BodyInterface& factory = body->context->physicsSystem->GetBodyInterface();
    factory.RemoveBody(body->id);
    factory.DestroyBody(body->id);
    delete body;
}

void game::Context::addClass(const meta::Class& cls) {
}

void game::Context::addObject(game::Object *object) {
}

void game::Context::destroyObject(game::Object& object) {

}

void game::Context::tick(float dt) {
    mImpl->debugRenderer->begin_frame(*mImpl->activeCamera);

    // TODO: account for <60fps
    int steps = 1; // std::max(1, int(ceilf(mTimeAccumulator / kTimeStep)));

    if (JPH::EPhysicsUpdateError err = mImpl->physicsSystem->Update(dt, steps, *mImpl->physicsAllocator, *mImpl->physicsThreadPool); err != JPH::EPhysicsUpdateError::None) {
        gPhysicsLog.warn("Physics update error: {}", std::to_underlying(err));
    }
}

void game::Context::shutdown() {
    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
}

void game::Context::setCamera(const draw::Camera& camera) {
    mImpl->activeCamera = &camera;
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

static void create_debug_pipeline(render::Pipeline& pipeline, const draw::ViewportConfig& config, render::Context& context) {
    {
        // mvp matrix
        CD3DX12_ROOT_PARAMETER1 params[1];
        params[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
        desc.Init_1_1(1, params, 0, nullptr, kPrimitiveRootFlags);

        context.serialize_root_signature(pipeline.signature, desc);
    }

    {
        auto ps = context.mConfig.bundle.get_shader_bytecode("jolt_debug.ps");
        auto vs = context.mConfig.bundle.get_shader_bytecode("jolt_debug.vs");

        constexpr D3D12_INPUT_ELEMENT_DESC kInputElements[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DebugVertex, position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOUR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(DebugVertex, colour), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        const D3D12_GRAPHICS_PIPELINE_STATE_DESC kDesc = {
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
            .RTVFormats = { config.colour },
            .DSVFormat = config.depth,
            .SampleDesc = { 1, 0 },
        };

        auto& device = context.mDevice;

        SM_ASSERT_HR(device->CreateGraphicsPipelineState(&kDesc, IID_PPV_ARGS(&pipeline.pso)));
    }
}

void game::physics_debug(
    graph::FrameGraph& graph,
    const draw::Camera& camera,
    graph::Handle target)
{
    auto config = camera.config();

    graph::PassBuilder pass = graph.graphics("Physics Debug");
    pass.write(target, "Target", graph::Usage::eRenderTarget);

    graph::ResourceInfo info = {
        .sz = graph::ResourceSize::tex2d(config.size),
        .format = config.depth,
        .clear = graph::clear_depth(1.0f)
    };

    graph::Handle depth = pass.create(info, "Depth", graph::Usage::eDepthWrite);

    auto& data = graph.device_data([config](render::Context& context) {
        struct {
            render::Pipeline pipeline;

            render::VertexBuffer<DebugVertex> vbo;
        } info;

        create_debug_pipeline(info.pipeline, config, context);

        info.vbo = context.vertex_upload_buffer<DebugVertex>(2048);

        return info;
    });

    pass.bind([target, depth, &data, &camera](graph::RenderContext& ctx) {
        auto& context = ctx.context;
        auto *cmd = ctx.commands;

        auto dsv = ctx.graph.dsv(depth);
        auto dsv_cpu = context.mDsvPool.cpu_handle(dsv);

        auto rtv = ctx.graph.rtv(target);
        auto rtv_cpu = context.mRtvPool.cpu_handle(rtv);

        cmd->SetGraphicsRootSignature(*data.pipeline.signature);
        cmd->SetPipelineState(*data.pipeline.pso);

        auto vp = camera.viewport();

        cmd->RSSetViewports(1, &vp.mViewport);
        cmd->RSSetScissorRects(1, &vp.mScissorRect);

        cmd->OMSetRenderTargets(1, &rtv_cpu, false, &dsv_cpu);

        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

        auto& debug = static_cast<CDebugRenderer&>(*JPH::DebugRenderer::sInstance);

        if (debug.mVertices.empty())
            return;

        data.vbo.update(debug.mVertices);

        D3D12_VERTEX_BUFFER_VIEW vbv = data.vbo.get_view();

        cmd->IASetVertexBuffers(0, 1, &vbv);

        cmd->DrawInstanced(debug.mVertices.size(), 1, 0, 0);
    });
}