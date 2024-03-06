#include "render/render.hpp"

#include "core/format.hpp" // IWYU pragma: export

#include "d3dx12/d3dx12_resource_helpers.h"
#include "render/draw.hpp" // IWYU pragma: export

#include "math/colour.hpp"

#include "DDSTextureLoader12.h"

#include "render/draw/imgui.hpp"
#include "render/draw/present.hpp"
#include "render/draw/scene.hpp"

#include "d3dx12/d3dx12_core.h"
#include "d3dx12/d3dx12_root_signature.h"
#include "d3dx12/d3dx12_barriers.h"

using namespace sm;
using namespace sm::render;

static auto gSink = logs::get_sink(logs::Category::eRender);

static uint get_swapchain_flags(const Instance& instance) {
    return instance.tearing_support() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
}

static logs::Severity get_severity(MessageSeverity sev) {
    using enum MessageSeverity::Inner;

    switch (sev.as_enum()) {
    case eMessage: return logs::Severity::eTrace;
    case eInfo: return logs::Severity::eInfo;
    case eWarning: return logs::Severity::eWarning;
    case eError: return logs::Severity::eError;
    case eCorruption: return logs::Severity::eFatal;

    default: return logs::Severity::eError;
    }
}

void Context::on_info(
    D3D12_MESSAGE_CATEGORY category,
    D3D12_MESSAGE_SEVERITY severity,
    D3D12_MESSAGE_ID id,
    LPCSTR desc,
    void *user)
{
    MessageCategory c{category};
    MessageSeverity s{severity};
    MessageID message{id};

    gSink.log(get_severity(s), "{} {}: {}", c, message, desc);
}

void Context::enable_info_queue() {
    if (Result hr = mDevice.query(&mInfoQueue)) {
        mInfoQueue->RegisterMessageCallback(&on_info, D3D12_MESSAGE_CALLBACK_FLAG_NONE, this, &mCookie);
    } else {
        gSink.warn("failed to query info queue: {}", hr);
    }
}

void Context::enable_debug_layer(bool gbv, bool rename) {
    if (Result hr = D3D12GetDebugInterface(IID_PPV_ARGS(&mDebug))) {
        mDebug->EnableDebugLayer();

        Object<ID3D12Debug3> debug3;
        if (mDebug.query(&debug3)) debug3->SetEnableGPUBasedValidation(gbv);
        else gSink.warn("failed to get debug3 interface: {}", hr);

        Object<ID3D12Debug5> debug5;
        if (mDebug.query(&debug5)) debug5->SetEnableAutoName(rename);
        else gSink.warn("failed to get debug5 interface: {}", hr);
    } else {
        gSink.warn("failed to get debug interface: {}", hr);
    }
}

void Context::disable_debug_layer() {
    Object<ID3D12Debug4> debug4;
    if (Result hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debug4))) {
        debug4->DisableDebugLayer();
    } else {
        gSink.warn("failed to query debug4 interface: {}", hr);
    }
}

void Context::enable_dred(bool enabled) {
    Object<ID3D12DeviceRemovedExtendedDataSettings> dred;
    if (Result hr = D3D12GetDebugInterface(IID_PPV_ARGS(&dred))) {
        dred->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        dred->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
    } else {
        gSink.warn("failed to query dred settings: {}", hr);
    }
}

void Context::query_root_signature_version() {
    mRootSignatureVersion = mFeatureSupport.HighestRootSignatureVersion();
    gSink.info("root signature version: {}", mRootSignatureVersion);
}

void Context::serialize_root_signature(Object<ID3D12RootSignature>& signature, const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc) {
    Blob serialized;
    Blob error;
    if (Result hr = D3DX12SerializeVersionedRootSignature(&desc, mRootSignatureVersion.as_facade(), &serialized, &error); !hr) {
        gSink.error("failed to serialize root signature: {}", hr);
        gSink.error("| error: {}", error.as_string());
        SM_ASSERT_HR(hr);
    }

    SM_ASSERT_HR(mDevice->CreateRootSignature(0, serialized.data(), serialized.size(), IID_PPV_ARGS(&signature)));
}

void Context::create_device(size_t index) {
    auto& adapter = mInstance.get_adapter(index);

    if (auto flags = adapter.flags(); flags.test(AdapterFlag::eSoftware) && mDebugFlags.test(DebugFlags::eGpuValidation)) {
        gSink.warn("adapter `{}` is a software adapter, enabling gpu validation has major performance implications", adapter.name());
    }

    if (mDebugFlags.test(DebugFlags::eDeviceDebugLayer))
        enable_debug_layer(mDebugFlags.test(DebugFlags::eGpuValidation), mDebugFlags.test(DebugFlags::eAutoName));
    else
        disable_debug_layer();

    enable_dred(mDebugFlags.test(DebugFlags::eDeviceRemovedInfo));

    auto fl = get_feature_level();

    mAdapterIndex = index;
    if (Result hr = D3D12CreateDevice(adapter.get(), fl.as_facade(), IID_PPV_ARGS(&mDevice)); !hr) {
        gSink.error("failed to create device `{}` at feature level `{}`", adapter.name(), fl, hr);
        gSink.error("| hresult: {}", hr);
        gSink.error("falling back to warp adapter...");

        mAdapterIndex = mInstance.warp_adapter_index();
        auto& warp_adapter = mInstance.get_adapter(mAdapterIndex);
        SM_ASSERT_HR(D3D12CreateDevice(warp_adapter.get(), fl.as_facade(), IID_PPV_ARGS(&mDevice)));
    }

    gSink.info("device created: {}", adapter.name());

    if (mDebugFlags.test(DebugFlags::eInfoQueue))
        enable_info_queue();

    SM_ASSERT_HR(mFeatureSupport.Init(*mDevice));

    query_root_signature_version();

    gSink.info("| feature level: {}", fl);
    gSink.info("| flags: {}", mDebugFlags);

    create_dstorage();
}

static constexpr D3D12MA::ALLOCATOR_FLAGS kAllocatorFlags = D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED | D3D12MA::ALLOCATOR_FLAG_SINGLETHREADED;

void Context::create_allocator() {
    const D3D12MA::ALLOCATOR_DESC kAllocatorDesc = {
        .Flags = kAllocatorFlags,
        .pDevice = mDevice.get(),
        .pAdapter = *mInstance.get_adapter(mAdapterIndex),
    };

    SM_ASSERT_HR(D3D12MA::CreateAllocator(&kAllocatorDesc, &mAllocator));
}

void Context::reset_direct_commands(ID3D12PipelineState *pso) {
    auto& allocator = mFrames[mFrameIndex].allocator;
    SM_ASSERT_HR(allocator->Reset());
    mCommandList.reset(*allocator, pso);
}

void Context::create_copy_queue() {
    constexpr D3D12_COMMAND_QUEUE_DESC kQueueDesc = {
        .Type = D3D12_COMMAND_LIST_TYPE_COPY,
    };

    SM_ASSERT_HR(mDevice->CreateCommandQueue(&kQueueDesc, IID_PPV_ARGS(&mCopyQueue)));

    SM_ASSERT_HR(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&mCopyAllocator)));
    SM_ASSERT_HR(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, mCopyAllocator.get(), nullptr, IID_PPV_ARGS(&mCopyCommands)));
    SM_ASSERT_HR(mCopyCommands->Close());
}

void Context::reset_copy_commands() {
    SM_ASSERT_HR(mCopyAllocator->Reset());
    SM_ASSERT_HR(mCopyCommands->Reset(*mCopyAllocator, nullptr));
}

void Context::create_copy_fence() {
    SM_ASSERT_HR(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mCopyFence)));
    mCopyFenceValue = 1;

    SM_ASSERT_WIN32(mCopyFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr));
}

void Context::create_pipeline() {
    constexpr D3D12_COMMAND_QUEUE_DESC kQueueDesc = {
        .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
    };

    SM_ASSERT_HR(mDevice->CreateCommandQueue(&kQueueDesc, IID_PPV_ARGS(&mDirectQueue)));

    bool tearing = mInstance.tearing_support();
    const DXGI_SWAP_CHAIN_DESC1 kSwapChainDesc = {
        .Width = mSwapChainSize.width,
        .Height = mSwapChainSize.height,
        .Format = mSwapChainFormat,
        .SampleDesc = { 1, 0 },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = mSwapChainLength,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .Flags = get_swapchain_flags(mInstance),
    };

    auto& factory = mInstance.factory();
    HWND hwnd = mConfig.window.get_handle();

    Object<IDXGISwapChain1> swapchain1;
    SM_ASSERT_HR(factory->CreateSwapChainForHwnd(*mDirectQueue, hwnd, &kSwapChainDesc, nullptr, nullptr, &swapchain1));

    if (tearing)
    {
        SM_ASSERT_HR(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
    }

    SM_ASSERT_HR(swapchain1.query(&mSwapChain));
    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

    SM_ASSERT_HR(mRtvPool.init(*mDevice, min_rtv_heap_size(), D3D12_DESCRIPTOR_HEAP_FLAG_NONE));
    SM_ASSERT_HR(mDsvPool.init(*mDevice, min_dsv_heap_size(), D3D12_DESCRIPTOR_HEAP_FLAG_NONE));
    SM_ASSERT_HR(mSrvPool.init(*mDevice, min_srv_heap_size(), D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE));

    mFrames.resize(mSwapChainLength);

    update_scene_viewport();
    create_frame_rtvs();
    create_render_targets();
    create_frame_allocators();

    for (uint i = 0; i < mSwapChainLength; i++) {
        mFrames[i].fence_value = 0;
    }
}

static const D3D12_HEAP_PROPERTIES kDefaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
static const D3D12_HEAP_PROPERTIES kUploadHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

void Context::create_frame_rtvs() {
    for (auto& frame : mFrames) {
        frame.rtv_index = mRtvPool.allocate();
    }
}

void Context::destroy_frame_rtvs() {
    for (auto& frame : mFrames) {
        mRtvPool.release(frame.rtv_index);
        frame.rtv_index = RtvIndex::eInvalid;
    }
}

void Context::create_render_targets() {
    CTASSERTF(mSwapChainLength <= DXGI_MAX_SWAP_CHAIN_BUFFERS, "too many swap chain buffers (%u > %u)", mSwapChainLength, DXGI_MAX_SWAP_CHAIN_BUFFERS);

    for (uint i = 0; i < mSwapChainLength; i++) {
        auto& backbuffer = mFrames[i].target;
        auto rtv = mRtvPool.cpu_handle(mFrames[i].rtv_index);
        SM_ASSERT_HR(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&backbuffer)));
        mDevice->CreateRenderTargetView(*backbuffer, nullptr, rtv);

        backbuffer.rename(fmt::format("BackBuffer {}", i));
    }
}

void Context::create_frame_allocators() {
    for (uint i = 0; i < mSwapChainLength; i++) {
        SM_ASSERT_HR(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mFrames[i].allocator)));
    }
}

void Context::update_scene_viewport() {
    mSceneViewport = Viewport{mSceneSize};
}

Result Context::create_resource(Resource& resource, D3D12_HEAP_TYPE heap, D3D12_RESOURCE_DESC desc, D3D12_RESOURCE_STATES state, const D3D12_CLEAR_VALUE *clear) {
    const D3D12MA::ALLOCATION_DESC kAllocDesc = {
        .HeapType = heap,
    };

    return mAllocator->CreateResource(&kAllocDesc, &desc, state, clear, &resource.mAllocation, IID_PPV_ARGS(&resource.mResource));
}

Result Context::load_dds_texture(Object<ID3D12Resource>& texture, sm::Vector<D3D12_SUBRESOURCE_DATA>& mips, const char *name) {
    auto data = mConfig.bundle.get_texture(name);
    Result hr = DirectX::LoadDDSTextureFromMemory(
        mDevice.get(),
        data.data(),
        data.size_bytes(),
        &texture,
        mips
    );

    if (hr.success()) {
        texture.rename(name);
        return S_OK;
    }

    gSink.error("failed to load dds texture: {}", hr);
    return hr;
}

void Context::copy_buffer(Object<ID3D12GraphicsCommandList1>& list, Resource& dst, Resource& src, size_t size) {
    list->CopyBufferRegion(*dst.mResource, 0, *src.mResource, 0, size);
}

static const D3D12_ROOT_SIGNATURE_FLAGS kPrimitiveRootFlags
    = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

void Context::create_primitive_pipeline() {
    {
        // mvp matrix
        CD3DX12_ROOT_PARAMETER1 params[1];
        params[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
        desc.Init_1_1(1, params, 0, nullptr, kPrimitiveRootFlags);

        serialize_root_signature(mPrimitivePipeline.signature, desc);
    }

    {
        auto ps = mConfig.bundle.get_shader_bytecode("primitive.ps");
        auto vs = mConfig.bundle.get_shader_bytecode("primitive.vs");

        constexpr D3D12_INPUT_ELEMENT_DESC kInputElements[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(draw::Vertex, position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOUR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, offsetof(draw::Vertex, colour), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        const D3D12_GRAPHICS_PIPELINE_STATE_DESC kDesc = {
            .pRootSignature = mPrimitivePipeline.signature.get(),
            .VS = CD3DX12_SHADER_BYTECODE(vs.data(), vs.size()),
            .PS = CD3DX12_SHADER_BYTECODE(ps.data(), ps.size()),
            .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
            .SampleMask = UINT_MAX,
            .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
            .DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
            .InputLayout = { kInputElements, _countof(kInputElements) },
            .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            .NumRenderTargets = 1,
            .RTVFormats = { mSceneFormat },
            .DSVFormat = mDepthFormat,
            .SampleDesc = { 1, 0 },
        };

        SM_ASSERT_HR(mDevice->CreateGraphicsPipelineState(&kDesc, IID_PPV_ARGS(&mPrimitivePipeline.pso)));
    }
}

void Context::destroy_primitive_pipeline() {
    mPrimitivePipeline.reset();
}

static constexpr draw::MeshInfo kInfo = {
    .type = draw::MeshType::eCube,
    .cube = {
        .width = 1.f,
        .height = 1.f,
        .depth = 1.f,
    },
};

void Context::init_scene() {
    mPrimitives.push_back(create_mesh(kInfo, float3(1.f, 0.f, 0.f)));
}

void Context::create_scene() {
    for (auto& primitive : mPrimitives) {
        primitive = create_mesh(primitive.mInfo, float3(1.f, 0.f, 0.f));
    }
}

void Context::destroy_scene() {
    for (auto& primitive : mPrimitives) {
        primitive.mVertexBuffer.reset();
        primitive.mIndexBuffer.reset();
    }
}

texindex Context::load_texture_stb(const fs::path& path) {
    auto image = open_image(path);
    if (image.size == 0u) return UINT16_MAX;

    Resource texture;
    Resource upload;

    const auto kTextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R8G8B8A8_UNORM,
        image.size.width,
        image.size.height,
        1, 1
    );

    SM_ASSERT_HR(create_resource(texture, D3D12_HEAP_TYPE_DEFAULT, kTextureDesc, D3D12_RESOURCE_STATE_COPY_DEST));

    const uint kUploadBufferSize = GetRequiredIntermediateSize(*texture.mResource, 0, 1);
    const auto kUploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(kUploadBufferSize);

    SM_ASSERT_HR(create_resource(upload, D3D12_HEAP_TYPE_UPLOAD, kUploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ));

    D3D12_RANGE read{0, 0};
    void *data;
    SM_ASSERT_HR(upload.map(&read, &data));
    std::memcpy(data, image.data.data(), kUploadBufferSize);
    upload.unmap(&read);

    reset_direct_commands();
    reset_copy_commands();

    const D3D12_SUBRESOURCE_DATA kTextureData = {
        .pData = image.data.data(),
        .RowPitch = int_cast<LONG_PTR>(image.size.width * 4),
        .SlicePitch = int_cast<LONG_PTR>(image.size.width * image.size.height * 4),
    };

    UpdateSubresources<1>(mCopyCommands.get(), *texture.mResource, upload.mResource.get(), 0, 0, 1, &kTextureData);

    const D3D12_RESOURCE_BARRIER kBarriers[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(
            *texture.mResource,
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        )
    };

    mCommandList.submit_barriers(kBarriers);

    SM_ASSERT_HR(mCopyCommands->Close());
    SM_ASSERT_HR(mCommandList->Close());

    ID3D12CommandList *copy_lists[] = { mCopyCommands.get() };
    mCopyQueue->ExecuteCommandLists(1, copy_lists);
    SM_ASSERT_HR(mCopyQueue->Signal(*mCopyFence, mCopyFenceValue));

    ID3D12CommandList *direct_lists[] = { mCommandList.get() };
    SM_ASSERT_HR(mCopyQueue->Wait(*mCopyFence, mCopyFenceValue));
    mDirectQueue->ExecuteCommandLists(1, direct_lists);

    flush_copy_queue();

    const D3D12_SHADER_RESOURCE_VIEW_DESC kSrvDesc = {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D = {
            .MostDetailedMip = 0,
            .MipLevels = 1,
            .ResourceMinLODClamp = 0.f,
        },
    };

    SrvIndex index = mSrvPool.allocate();
    auto cpu = mSrvPool.cpu_handle(index);
    mDevice->CreateShaderResourceView(*texture.mResource, &kSrvDesc, cpu);

    texindex texid = int_cast<texindex>(mTextures.size());

    mTextures.emplace_back(Texture {
        .name = path.string(),
        .format = ImageFormat::ePNG,
        .size = image.size,
        .mips = 1,

        .resource = std::move(texture),
        .srv = index,
    });

    return texid;
}

texindex Context::load_texture_dds(const fs::path& path) {
    Object<ID3D12Resource> texture;
    sm::Vector<D3D12_SUBRESOURCE_DATA> mips;
    std::unique_ptr<uint8_t[]> data;

    auto wpath = path.wstring();

    Result hr = DirectX::LoadDDSTextureFromFile(
        mDevice.get(),
        wpath.c_str(),
        &texture,
        data,
        mips);

    if (hr.failed()) {
        gSink.error("failed to load dds texture: {}", hr);
        return UINT16_MAX;
    }

    const uint kUploadBufferSize = GetRequiredIntermediateSize(*texture, 0, int_cast<uint>(mips.size()));
    const auto kUploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(kUploadBufferSize);

    Resource upload;

    SM_ASSERT_HR(create_resource(upload, D3D12_HEAP_TYPE_UPLOAD, kUploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ));

    // TODO: really need an async copy queue on another thread
    wait_for_gpu();
    flush_copy_queue();

    reset_copy_commands();
    reset_direct_commands();

    UpdateSubresources<16>(mCopyCommands.get(), texture.get(), upload.get(), 0, 0, int_cast<uint>(mips.size()), mips.data());

    const D3D12_RESOURCE_BARRIER kBarriers[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(
            *texture,
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        )
    };

    mCommandList.submit_barriers(kBarriers);

    SM_ASSERT_HR(mCopyCommands->Close());
    SM_ASSERT_HR(mCommandList->Close());

    ID3D12CommandList *copy_lists[] = { mCopyCommands.get() };
    mCopyQueue->ExecuteCommandLists(1, copy_lists);
    SM_ASSERT_HR(mCopyQueue->Signal(*mCopyFence, mCopyFenceValue));

    ID3D12CommandList *direct_lists[] = { mCommandList.get() };
    SM_ASSERT_HR(mCopyQueue->Wait(*mCopyFence, mCopyFenceValue));
    mDirectQueue->ExecuteCommandLists(1, direct_lists);

    wait_for_gpu();
    flush_copy_queue();

    texindex texid = int_cast<texindex>(mTextures.size());
    return texid;
}

texindex Context::load_texture(const fs::path& path, ImageFormat type) {
    auto str = path.string();
    using enum ImageFormat::Inner;
    switch (type) {
    case eUnknown:
        if (texindex i = load_texture_stb(str.c_str()); i != UINT16_MAX)
            return i;
        if (texindex i = load_texture_dds(str.c_str()); i != UINT16_MAX)
            return i;
        gSink.error("unsupported image type: {}", type);
        break;
    case ePNG:
    case eJPG:
        return load_texture_stb(str.c_str());

    case eBC7:
        return load_texture_dds(str.c_str());

    default:
        gSink.error("unsupported image type: {}", type);
        return UINT16_MAX;
    }

    return UINT16_MAX;
}

Context::Primitive Context::create_mesh(const draw::MeshInfo& info, const float3& colour) {
    auto mesh = draw::primitive(info);

    uint32_t col = pack_colour(float4(colour, 1.0f));

    for (auto& v : mesh.vertices) {
        v.colour = col;
    }

    Resource vbo_upload;
    Resource ibo_upload;

    const uint kVertexBufferSize = mesh.vertices.size() * sizeof(draw::Vertex);
    const uint kIndexBufferSize = mesh.indices.size() * sizeof(uint16);

    const auto kVertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(kVertexBufferSize);
    const auto kIndexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(kIndexBufferSize);

    Primitive primitive = {
        .mInfo = info,
        .mIndexCount = int_cast<uint>(mesh.indices.size()),
    };

    SM_ASSERT_HR(create_resource(vbo_upload, D3D12_HEAP_TYPE_UPLOAD, kVertexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ));
    SM_ASSERT_HR(create_resource(ibo_upload, D3D12_HEAP_TYPE_UPLOAD, kIndexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ));

    D3D12_RANGE read{0, 0};

    {
        void *data;
        SM_ASSERT_HR(vbo_upload.map(&read, &data));
        std::memcpy(data, mesh.vertices.data(), kVertexBufferSize);
        vbo_upload.unmap(&read);
    }

    {
        void *data;
        SM_ASSERT_HR(ibo_upload.map(&read, &data));
        std::memcpy(data, mesh.indices.data(), kIndexBufferSize);
        ibo_upload.unmap(&read);
    }

    SM_ASSERT_HR(create_resource(primitive.mVertexBuffer, D3D12_HEAP_TYPE_DEFAULT, kVertexBufferDesc, D3D12_RESOURCE_STATE_COMMON));
    SM_ASSERT_HR(create_resource(primitive.mIndexBuffer, D3D12_HEAP_TYPE_DEFAULT, kIndexBufferDesc, D3D12_RESOURCE_STATE_COMMON));

    reset_direct_commands();
    reset_copy_commands();

    copy_buffer(mCopyCommands, primitive.mVertexBuffer, vbo_upload, kVertexBufferSize);
    copy_buffer(mCopyCommands, primitive.mIndexBuffer, ibo_upload, kIndexBufferSize);

    const D3D12_RESOURCE_BARRIER kBarriers[2] = {
        CD3DX12_RESOURCE_BARRIER::Transition(
            *primitive.mVertexBuffer.mResource,
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        ),
        CD3DX12_RESOURCE_BARRIER::Transition(
            *primitive.mIndexBuffer.mResource,
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_INDEX_BUFFER
        )
    };

    mCommandList->ResourceBarrier(_countof(kBarriers), kBarriers);

    SM_ASSERT_HR(mCopyCommands->Close());
    SM_ASSERT_HR(mCommandList->Close());

    ID3D12CommandList *copy_lists[] = { mCopyCommands.get() };
    mCopyQueue->ExecuteCommandLists(1, copy_lists);
    mCopyQueue->Signal(*mCopyFence, mCopyFenceValue);

    ID3D12CommandList *direct_lists[] = { mCommandList.get() };
    mDirectQueue->Wait(*mCopyFence, mCopyFenceValue);
    mDirectQueue->ExecuteCommandLists(1, direct_lists);

    primitive.mVertexBufferView = {
        .BufferLocation = primitive.mVertexBuffer.get_gpu_address(),
        .SizeInBytes = kVertexBufferSize,
        .StrideInBytes = sizeof(draw::Vertex),
    };

    primitive.mIndexBufferView = {
        .BufferLocation = primitive.mIndexBuffer.get_gpu_address(),
        .SizeInBytes = kIndexBufferSize,
        .Format = DXGI_FORMAT_R16_UINT,
    };

    wait_for_gpu();
    flush_copy_queue();

    return primitive;
}

void Context::create_assets() {
    create_primitive_pipeline();
    create_blit_pipeline();

    SM_ASSERT_HR(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, *mFrames[mFrameIndex].allocator, nullptr, IID_PPV_ARGS(&mCommandList)));
    SM_ASSERT_HR(mCommandList->Close());

    SM_ASSERT_HR(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
    mFrames[mFrameIndex].fence_value += 1;

    SM_ASSERT_WIN32(mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr));
}

void Context::build_command_list() {
    mAllocator->SetCurrentFrameIndex(mFrameIndex);

    reset_direct_commands();

    auto& [backbuffer, _0, rtv, _1] = mFrames[mFrameIndex];

    mFrameGraph.update(mSwapChainHandle, backbuffer.get());
    mFrameGraph.update(mSwapChainHandle, rtv);

    {
        ID3D12DescriptorHeap *heaps[] = { mSrvPool.get() };
        mCommandList->SetDescriptorHeaps(_countof(heaps), heaps);

        mFrameGraph.execute();
    }

    SM_ASSERT_HR(mCommandList->Close());
}

void Context::create_frame_graph() {
    {
        graph::TextureInfo info = { .name = "SwapChain", .size = mSwapChainSize, .format = mSwapChainFormat };
        mSwapChainHandle = mFrameGraph.include(info, graph::Access::ePresent, nullptr);
    }

    draw::draw_scene(mFrameGraph, mSceneTargetHandle);
    draw::draw_present(mFrameGraph, mSwapChainHandle, mSceneTargetHandle);
    draw::draw_imgui(mFrameGraph, mSwapChainHandle);

    mFrameGraph.compile();
}

void Context::destroy_frame_graph() {
    mFrameGraph.reset();
}

void Context::destroy_device() {
    destroy_frame_graph();

    // release frame resources
    for (uint i = 0; i < mSwapChainLength; i++) {
        mFrames[i].target.reset();
        mFrames[i].allocator.reset();
    }

    // release sync objects
    mFence.reset();
    mCopyFence.reset();

    // assets
    destroy_scene();
    destroy_screen_quad();

    // pipeline state
    destroy_primitive_pipeline();
    destroy_blit_pipeline();

    // copy commands
    mCopyCommands.reset();
    mCopyAllocator.reset();
    mCopyQueue.reset();

    // direct commands
    mDirectQueue.reset();
    mCommandList.reset();

    // allocator
    mAllocator.reset();

    // descriptor heaps
    mRtvPool.reset();
    mDsvPool.reset();
    mSrvPool.reset();

    // swapchain
    mSwapChain.reset();

    // device
    mDebug.reset();
    mInfoQueue.reset();
    mDevice.reset();
}

Context::Context(const RenderConfig& config)
    : mConfig(config)
    , mDebugFlags(config.flags)
    , mInstance({ mDebugFlags, config.preference })
    , mSwapChainFormat(config.swapchain_format)
    , mSwapChainSize(config.swapchain_size)
    , mSwapChainLength(config.swapchain_length)
    , mSceneFormat(config.scene_format)
    , mDepthFormat(config.depth_format)
    , mSceneSize(config.scene_size)
    , mPresentViewport(mSwapChainSize)
    , mSceneViewport(mSceneSize)
    , mFrameGraph(*this)
    , mEditor(*this)
{ }

void Context::create() {
    size_t index = mDebugFlags.test(DebugFlags::eWarpAdapter)
        ? mInstance.warp_adapter_index()
        : mConfig.adapter_index;

    mConfig.bundle.get_font("public_sans");

    mWorld = world::empty_world("test_world");

    create_device(index);
    create_allocator();
    create_copy_queue();
    create_copy_fence();
    create_pipeline();
    create_assets();

    create_screen_quad();
    init_scene();
    create_frame_graph();
    create_imgui();
}

void Context::destroy() {
    flush_copy_queue();
    wait_for_gpu();

    destroy_frame_graph();
    destroy_imgui();

    SM_ASSERT_WIN32(CloseHandle(mCopyFenceEvent));
    SM_ASSERT_WIN32(CloseHandle(mFenceEvent));
}

bool Context::update() {
    return update_imgui();
}

void Context::render() {
    build_command_list();

    ID3D12CommandList *lists[] = { mCommandList.get() };
    mDirectQueue->ExecuteCommandLists(1, lists);

    bool tearing = mInstance.tearing_support();
    uint flags = tearing ? DXGI_PRESENT_ALLOW_TEARING : 0;

    SM_ASSERT_HR(mSwapChain->Present(0, flags));

    move_to_next_frame();
}

void Context::update_adapter(size_t index) {
    if (index == mAdapterIndex) return;
    mAdapterIndex = index;

    recreate_device();
}

void Context::recreate_device() {
    wait_for_gpu();
    destroy_imgui_backend();
    destroy_frame_graph();

    // TODO: we dont actually need to destroy the dstorage factory
    // just the queue, its also slow to recreate the factory.
    // might be worth handling that case in the future
    destroy_dstorage();
    destroy_device();

    create_device(mAdapterIndex);
    create_allocator();
    create_copy_queue();
    create_copy_fence();
    create_pipeline();
    create_assets();

    create_screen_quad();
    create_scene();
    create_frame_graph();

    create_imgui_backend();

    mDeviceLost = false;
}

void Context::set_device_lost() {
    mDeviceLost = true;
}

void Context::update_swapchain_length(uint length) {
    wait_for_gpu();

    uint64 current = mFrames[mFrameIndex].fence_value;

    for (auto& frame : mFrames) {
        frame.target.reset();
        frame.allocator.reset();
    }

    destroy_frame_rtvs();

    const uint flags = get_swapchain_flags(mInstance);
    SM_ASSERT_HR(mSwapChain->ResizeBuffers(length, mSwapChainSize.width, mSwapChainSize.height, mSwapChainFormat, flags));
    mSwapChainLength = length;

    mFrames.resize(length);
    for (uint i = 0; i < length; i++) {
        mFrames[i].fence_value = current;
    }

    create_frame_rtvs();
    create_render_targets();
    create_frame_allocators();

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
}

void Context::resize_swapchain(math::uint2 size) {
    wait_for_gpu();

    for (uint i = 0; i < mSwapChainLength; i++) {
        mFrames[i].target.reset();
        mFrames[i].fence_value = mFrames[mFrameIndex].fence_value;
    }

    destroy_frame_graph();
    destroy_frame_rtvs();

    const uint flags = get_swapchain_flags(mInstance);
    SM_ASSERT_HR(mSwapChain->ResizeBuffers(mSwapChainLength, size.width, size.height, mSwapChainFormat, flags));
    mSwapChainSize = size;

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

    create_frame_rtvs();
    update_display_viewport();
    create_render_targets();
    create_frame_graph();
}

void Context::update_scene_size(math::uint2 size) {
    if (mSceneSize == size) return;

    resize_draw(size);
}

void Context::resize_draw(math::uint2 size) {
    wait_for_gpu();

    destroy_frame_graph();

    mSceneSize = size;

    update_scene_viewport();
    update_display_viewport();
    create_frame_graph();
}

void Context::move_to_next_frame() {
    const uint64 current = mFrames[mFrameIndex].fence_value;
    SM_ASSERT_HR(mDirectQueue->Signal(*mFence, current));

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

    if (mFence->GetCompletedValue() < mFrames[mFrameIndex].fence_value) {
        SM_ASSERT_HR(mFence->SetEventOnCompletion(mFrames[mFrameIndex].fence_value, mFenceEvent));
        WaitForSingleObject(mFenceEvent, INFINITE);
    }

    mFrames[mFrameIndex].fence_value = current + 1;
}

void Context::wait_for_gpu() {
    const uint64 current = mFrames[mFrameIndex].fence_value++;
    SM_ASSERT_HR(mDirectQueue->Signal(*mFence, current));

    SM_ASSERT_HR(mFence->SetEventOnCompletion(current, mFenceEvent));
    WaitForSingleObject(mFenceEvent, INFINITE);
}

void Context::flush_copy_queue() {
    const uint64 current = mCopyFenceValue++;
    SM_ASSERT_HR(mCopyQueue->Signal(*mCopyFence, current));

    const uint64 completed = mCopyFence->GetCompletedValue();

    if (completed < current) {
        SM_ASSERT_HR(mCopyFence->SetEventOnCompletion(current, mCopyFenceEvent));
        WaitForSingleObject(mCopyFenceEvent, INFINITE);
    }
}
