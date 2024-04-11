#include "math/colour.hpp"
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

LOG_CATEGORY_IMPL(gPhysicsLogs, "physics")
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
        gPhysicsLogs.info("Body activated: {}", id.GetIndex());
    }

    void OnBodyDeactivated(const JPH::BodyID& id, uint64 user) override {
        gPhysicsLogs.info("Body deactivated: {}", id.GetIndex());
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
        float3 from = { inFrom.GetX(), inFrom.GetY(), inFrom.GetZ() };
        float3 to = { inTo.GetX(), inTo.GetY(), inTo.GetZ() };
        float3 colour = math::unpack_colour(inColor.GetUInt32()).xyz();

        mVertices.push_back({ from, colour });
        mVertices.push_back({ to, colour });
    }

    void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) override {

    }

    void begin_frame(const draw::Camera& camera) {
        mVertices.clear();

        float3 p = camera.position();

        Super::SetCameraPos(JPH::RVec3(p.x, p.y, p.z));
    }
};

static char gTraceBuffer[2048];

static void jph_trace(const char *fmt, ...) { // NOLINT
    va_list args;
    va_start(args, fmt);
    str_sprintf(gTraceBuffer, sizeof(gTraceBuffer), fmt, args);
    va_end(args);

    gPhysicsLogs.trace("{}", gTraceBuffer);
}

static bool jph_assert(const char *expr, const char *message, const char *file, uint line) {
    gPhysicsLogs.panic("Assertion failed: {} in {}:{}\n{}", expr, file, line, message);
    return true;
}

class Context final : public game::IContext {
    [[maybe_unused]] world::World& mWorld;

    constexpr static float kTimeStep = 1.0f / 60.0f;
    float mTimeAccumulator = 0.0f;

    void tick(float dt) override {
        mDebugRenderer->begin_frame(*mActiveCamera);

        mTimeAccumulator += dt;

        int steps = int(mTimeAccumulator / kTimeStep);

        if (JPH::EPhysicsUpdateError err = mPhysicsSystem->Update(dt, steps, *mPhysicsAllocator, *mPhysicsThreadPool); err != JPH::EPhysicsUpdateError::None) {
            gPhysicsLogs.warn("Physics update error: {}", std::to_underlying(err));
        }

        mTimeAccumulator -= steps * kTimeStep;
    }

    void shutdown() override {
        shutdown_jph();
    }

    void set_camera(const draw::Camera& camera) override {
        mActiveCamera = &camera;
    }

    const draw::Camera *mActiveCamera = nullptr;

    ///
    /// jolt physics
    ///

    sm::UniquePtr<JPH::TempAllocatorImpl> mPhysicsAllocator;
    sm::UniquePtr<JPH::JobSystemThreadPool> mPhysicsThreadPool;
    sm::UniquePtr<JPH::PhysicsSystem> mPhysicsSystem;

    sm::UniquePtr<CBroadPhaseLayer> mBroadPhaseLayer;
    sm::UniquePtr<CObjectLayerPairFilter> mObjectLayerPairFilter;
    sm::UniquePtr<CObjectVsBroadPhaseFilter> mObjectVsBroadPhaseLayerFilter;
    sm::UniquePtr<CContactListener> mContactListener;
    sm::UniquePtr<CBodyActivationListener> mBodyActivationListener;

    sm::UniquePtr<CDebugRenderer> mDebugRenderer;

    void shutdown_jph() {
        JPH::UnregisterTypes();

        delete JPH::Factory::sInstance;
    }

    void init_jph() {
        JPH::RegisterDefaultAllocator();

        JPH::Trace = jph_trace;
        JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = jph_assert;)
        JPH::Factory::sInstance = new JPH::Factory();

        mDebugRenderer = sm::make_unique<CDebugRenderer>();
        JPH::DebugRenderer::sInstance = mDebugRenderer.get();

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

        create_physics_bodies();

        mPhysicsSystem->OptimizeBroadPhase();
    }

    void create_physics_bodies() {
        // JPH::BodyInterface& body = mPhysicsSystem->GetBodyInterface();
    }

public:
    Context(world::World& world)
        : mWorld(world)
    {
        init_jph();
    }
};

game::IContext& game::init(world::World& world, const draw::Camera& camera) {
    CTASSERTF(gContext == nullptr, "Context already initialized");

    gContext = new Context(world);

    gContext->set_camera(camera);

    return IContext::get();
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
    graph::Handle target,
    graph::Handle depth)
{
    graph::PassBuilder pass = graph.graphics("Physics Debug");
    pass.write(target, "Target", graph::Usage::eRenderTarget);

    auto config = camera.config();

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