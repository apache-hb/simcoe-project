#include "stdafx.hpp"

#include "render/render.hpp"

using namespace sm;
using namespace sm::render;

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

    logs::gRender.log(get_severity(s), "{} {}: {}", c, message, desc);
}

void Context::enable_info_queue() {
    if (Result hr = mDevice.query(&mInfoQueue)) {
        mInfoQueue->RegisterMessageCallback(&on_info, D3D12_MESSAGE_CALLBACK_FLAG_NONE, this, &mCookie);
        mInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        mInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
    } else {
        logs::gRender.warn("failed to query info queue: {}", hr);
    }
}

void Context::enable_debug_layer(bool gbv, bool rename) {
    if (Result hr = D3D12GetDebugInterface(IID_PPV_ARGS(&mDebug))) {
        mDebug->EnableDebugLayer();

        Object<ID3D12Debug3> debug3;
        if (mDebug.query(&debug3)) debug3->SetEnableGPUBasedValidation(gbv);
        else logs::gRender.warn("failed to get debug3 interface: {}", hr);

        Object<ID3D12Debug5> debug5;
        if (mDebug.query(&debug5)) debug5->SetEnableAutoName(rename);
        else logs::gRender.warn("failed to get debug5 interface: {}", hr);
    } else {
        logs::gRender.warn("failed to get debug interface: {}", hr);
    }
}

void Context::disable_debug_layer() {
    Object<ID3D12Debug4> debug4;
    if (Result hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debug4))) {
        debug4->DisableDebugLayer();
    } else {
        logs::gRender.warn("failed to query debug4 interface: {}", hr);
    }
}

void Context::enable_dred(bool enabled) {
    Object<ID3D12DeviceRemovedExtendedDataSettings> dred;
    if (Result hr = D3D12GetDebugInterface(IID_PPV_ARGS(&dred))) {
        dred->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        dred->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
    } else {
        logs::gRender.warn("failed to query dred settings: {}", hr);
    }
}

void Context::query_root_signature_version() {
    mRootSignatureVersion = mFeatureSupport.HighestRootSignatureVersion();
    logs::gRender.info("root signature version: {}", mRootSignatureVersion);
}

void Context::serialize_root_signature(Object<ID3D12RootSignature>& signature, const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc) {
    Blob serialized;
    Blob error;
    if (Result hr = D3DX12SerializeVersionedRootSignature(&desc, mRootSignatureVersion.as_facade(), &serialized, &error); !hr) {
        logs::gRender.error("failed to serialize root signature: {}", hr);
        logs::gRender.error("| error: {}", error.as_string());
        SM_ASSERT_HR(hr);
    }

    SM_ASSERT_HR(mDevice->CreateRootSignature(0, serialized.data(), serialized.size(), IID_PPV_ARGS(&signature)));
}

void Context::create_device(Adapter& adapter) {
    if (auto flags = adapter.flags(); flags.test(AdapterFlag::eSoftware) && mDebugFlags.test(DebugFlags::eGpuValidation)) {
        logs::gRender.warn("adapter `{}` is a software adapter, enabling gpu validation has major performance implications", adapter.name());
    }

    if (mDebugFlags.test(DebugFlags::eDeviceDebugLayer))
        enable_debug_layer(mDebugFlags.test(DebugFlags::eGpuValidation), mDebugFlags.test(DebugFlags::eAutoName));
    else
        disable_debug_layer();

    enable_dred(mDebugFlags.test(DebugFlags::eDeviceRemovedInfo));

    auto fl = get_feature_level();

    set_current_adapter(adapter);
    if (Result hr = D3D12CreateDevice(adapter.get(), fl.as_facade(), IID_PPV_ARGS(&mDevice)); !hr) {
        logs::gRender.error("failed to create device `{}` at feature level `{}`", adapter.name(), fl, hr);
        logs::gRender.error("| hresult: {}", hr);
        logs::gRender.error("falling back to warp adapter...");

        auto& warp = mInstance.get_warp_adapter();
        set_current_adapter(warp);
        SM_ASSERT_HR(D3D12CreateDevice(warp.get(), fl.as_facade(), IID_PPV_ARGS(&mDevice)));
    }

    logs::gRender.info("device created: {}", mCurrentAdapter->name());

    if (mDebugFlags.test(DebugFlags::eInfoQueue))
        enable_info_queue();

    SM_ASSERT_HR(mFeatureSupport.Init(*mDevice));

    query_root_signature_version();

    logs::gRender.info("| feature level: {}", fl);
    logs::gRender.info("| flags: {}", mDebugFlags);

    mStorage.create_queues(mDevice.get());
}

static constexpr D3D12MA::ALLOCATOR_FLAGS kAllocatorFlags = D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED | D3D12MA::ALLOCATOR_FLAG_SINGLETHREADED;

void Context::create_allocator() {
    const D3D12MA::ALLOCATOR_DESC kAllocatorDesc = {
        .Flags = kAllocatorFlags,
        .pDevice = mDevice.get(),
        .pAdapter = mCurrentAdapter->get()
    };

    SM_ASSERT_HR(D3D12MA::CreateAllocator(&kAllocatorDesc, &mAllocator));
}

void Context::reset_direct_commands(ID3D12PipelineState *pso) {
    auto& allocator = mFrames[mFrameIndex].allocator;
    SM_ASSERT_HR(allocator->Reset());
    SM_ASSERT_HR(mCommandList->Reset(*allocator, pso));
}

void Context::create_compute_queue() {
    constexpr D3D12_COMMAND_QUEUE_DESC kQueueDesc = {
        .Type = D3D12_COMMAND_LIST_TYPE_COMPUTE,
    };

    SM_ASSERT_HR(mDevice->CreateCommandQueue(&kQueueDesc, IID_PPV_ARGS(&mComputeQueue)));
}

void Context::destroy_compute_queue() {
    mComputeQueue.reset();
}

void Context::create_device_fence() {
    SM_ASSERT_HR(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mDeviceFence)));
    mDeviceFenceValue = 1;

    mDeviceFence.rename("Device Fence");
}

void Context::destroy_device_fence() {
    mDeviceFence.reset();
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

void Context::destroy_copy_queue() {
    mCopyCommands.reset();
    mCopyAllocator.reset();
    mCopyQueue.reset();
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

ID3D12CommandQueue *Context::get_queue(CommandListType type) {
    switch (type) {
    case CommandListType::eDirect: return mDirectQueue.get();
    case CommandListType::eCompute: return mComputeQueue.get();
    case CommandListType::eCopy: return mCopyQueue.get();

    default: CT_NEVER("invalid command list type");
    }
}

void Context::create_pipeline() {
    constexpr D3D12_COMMAND_QUEUE_DESC kQueueDesc = {
        .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
    };

    SM_ASSERT_HR(mDevice->CreateCommandQueue(&kQueueDesc, IID_PPV_ARGS(&mDirectQueue)));

    bool tearing = mInstance.tearing_support();
    const DXGI_SWAP_CHAIN_DESC1 kSwapChainDesc = {
        .Width = mSwapChainConfig.size.width,
        .Height = mSwapChainConfig.size.height,
        .Format = mSwapChainConfig.format,
        .SampleDesc = { 1, 0 },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = mSwapChainConfig.length,
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

    mFrames.resize(mSwapChainConfig.length);

    create_frame_rtvs();
    create_render_targets();
    create_frame_allocators();

    for (uint i = 0; i < mSwapChainConfig.length; i++) {
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
    CTASSERTF(mSwapChainConfig.length <= DXGI_MAX_SWAP_CHAIN_BUFFERS, "too many swap chain buffers (%u > %u)", mSwapChainConfig.length, DXGI_MAX_SWAP_CHAIN_BUFFERS);

    for (uint i = 0; i < mSwapChainConfig.length; i++) {
        auto& backbuffer = mFrames[i].target;
        auto rtv = mRtvPool.cpu_handle(mFrames[i].rtv_index);
        SM_ASSERT_HR(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&backbuffer)));
        mDevice->CreateRenderTargetView(*backbuffer, nullptr, rtv);

        backbuffer.rename(fmt::format("BackBuffer {}", i));
    }
}

void Context::create_frame_allocators() {
    for (uint i = 0; i < mSwapChainConfig.length; i++) {
        SM_ASSERT_HR(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mFrames[i].allocator)));
    }
}

Result Context::create_resource(Resource& resource, D3D12_HEAP_TYPE heap, D3D12_RESOURCE_DESC desc, D3D12_RESOURCE_STATES state, const D3D12_CLEAR_VALUE *clear) {
    const D3D12MA::ALLOCATION_DESC kAllocDesc = {
        .HeapType = heap,
    };

    return mAllocator->CreateResource(&kAllocDesc, &desc, state, clear, &resource.mAllocation, IID_PPV_ARGS(&resource.mResource));
}

void Context::copy_buffer(ID3D12GraphicsCommandList1 *list, Resource& dst, Resource& src, size_t size) {
    list->CopyBufferRegion(*dst.mResource, 0, *src.mResource, 0, size);
}

void Context::create_lights() {
    auto desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(PointLight) * mConfig.max_lights);
    SM_ASSERT_HR(create_resource(mPointLights.resource, D3D12_HEAP_TYPE_DEFAULT, desc, D3D12_RESOURCE_STATE_GENERIC_READ));
    SM_ASSERT_HR(create_resource(mPointLights.upload, D3D12_HEAP_TYPE_UPLOAD, desc, D3D12_RESOURCE_STATE_GENERIC_READ));

    D3D12_RANGE read{0, 0};
    SM_ASSERT_HR(mPointLights.upload.map(&read, &mPointLights.mapped));

    mPointLights.lights = { reinterpret_cast<PointLight *>(mPointLights.mapped), mConfig.max_lights };
}

void Context::destroy_lights() {
    mPointLights.resource.reset();
    mPointLights.upload.reset();
    mPointLights.mapped = nullptr;
}

static render::PointLight make_point_light(const world::PointLight& it, float3 position) {
    render::PointLight light = {
        .position = position,
        .colour = it.colour,
        .intensity = it.intensity,
    };

    return light;
}

void Context::update_node_lights(world::IndexOf<world::Node> node, uint& index, const float4x4& parent) {
    auto& info = mWorld.get(node);
    auto local = parent * info.transform.matrix();
    float3 position = local.get_translation();

    for (auto light : info.lights) {
        auto& data = mWorld.get(light);

        if (std::holds_alternative<world::PointLight>(data.light)) {
            auto& light = std::get<world::PointLight>(data.light);

            mPointLights.lights[index++] = make_point_light(light, position);
        }
    }

    for (auto child : info.children) {
        update_node_lights(child, index, local);
    }
}

void Context::update_lights(ID3D12GraphicsCommandList1 *list) {
    auto root = mWorld.get(mCurrentScene).root;

    uint index = 0;
    update_node_lights(root, index, float4x4::identity());
    mPointLights.count = index;

    copy_buffer(list, mPointLights.resource, mPointLights.upload, sizeof(PointLight) * index);
}

void Context::init_scene() {
    mWorld = world::default_world("Default World");
    mCurrentScene = mWorld.default_scene;
}

void Context::upload_buffer_view(RequestBuilder& request, const world::BufferView& view) {
    if (world::IndexOf file = world::get<world::File>(view.source); file != world::kInvalidIndex) {
        request.src(get_storage_file(file), view.offset, view.source_size);
        mStorage.submit_file_copy(request);
    } else if (world::IndexOf buffer = world::get<world::Buffer>(view.source); buffer != world::kInvalidIndex) {
        request.src(get_storage_buffer(buffer) + view.offset, view.source_size);
        mStorage.submit_memory_copy(request);
    } else {
        CT_NEVER("invalid image source");
    }
}

void Context::load_image(world::IndexOf<world::Image> index) {
    const auto& image = mWorld.get(index);

    Resource data;
    auto desc = CD3DX12_RESOURCE_DESC::Tex2D(image.format, image.size.width, image.size.height, 1, image.mips);
    SM_ASSERT_HR(create_resource(data, D3D12_HEAP_TYPE_DEFAULT, desc, D3D12_RESOURCE_STATE_COPY_DEST));

    RequestBuilder request;
    request.dst(DSTORAGE_DESTINATION_MULTIPLE_SUBRESOURCES{ data.get(), 0 });

    upload_buffer_view(request, image.source);

    mStorage.signal_file_queue(*mCopyFence, 1);
    mStorage.signal_memory_queue(*mCopyFence, 2);

    mStorage.flush_queues();

    reset_direct_commands();

    const D3D12_RESOURCE_BARRIER kBarriers[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(data.get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
    };

    mCommandList->ResourceBarrier(_countof(kBarriers), kBarriers);

    SM_ASSERT_HR(mCommandList->Close());

    ID3D12CommandList *lists[] = { mCommandList.get() };
    mDirectQueue->Wait(*mCopyFence, 2);
    mDirectQueue->ExecuteCommandLists(1, lists);

    mDirectQueue->Signal(*mCopyFence, 3);

    uint64 completed = mCopyFence->GetCompletedValue();
    if (completed < 3) {
        mCopyFence->SetEventOnCompletion(3, mCopyFenceEvent);
        WaitForSingleObject(mCopyFenceEvent, INFINITE);
    }

    SrvIndex srv = mSrvPool.allocate();
    auto cpu = mSrvPool.cpu_handle(srv);

    mDevice->CreateShaderResourceView(data.get(), nullptr, cpu);

    TextureResource resource = {
        .resource = std::move(data),
        .srv = srv,
    };

    mImages.emplace(index, std::move(resource));
}

void Context::load_mesh_buffer(world::IndexOf<world::Model> index, const world::Mesh& mesh) {
    Resource vbo_upload;
    Resource ibo_upload;

    size_t vbo_size = mesh.vertices.size() * sizeof(world::Vertex);
    size_t ibo_size = mesh.indices.size() * sizeof(uint16);

    auto vbo_desc = CD3DX12_RESOURCE_DESC::Buffer(vbo_size);
    auto ibo_desc = CD3DX12_RESOURCE_DESC::Buffer(ibo_size);
    SM_ASSERT_HR(create_resource(vbo_upload, D3D12_HEAP_TYPE_UPLOAD, vbo_desc, D3D12_RESOURCE_STATE_COMMON));
    SM_ASSERT_HR(create_resource(ibo_upload, D3D12_HEAP_TYPE_UPLOAD, ibo_desc, D3D12_RESOURCE_STATE_COMMON));

    Resource vbo;
    Resource ibo;
    SM_ASSERT_HR(create_resource(vbo, D3D12_HEAP_TYPE_DEFAULT, vbo_desc, D3D12_RESOURCE_STATE_COMMON));
    SM_ASSERT_HR(create_resource(ibo, D3D12_HEAP_TYPE_DEFAULT, ibo_desc, D3D12_RESOURCE_STATE_COMMON));

    SM_ASSERT_HR(vbo_upload.write(mesh.vertices.data(), vbo_size));
    SM_ASSERT_HR(ibo_upload.write(mesh.indices.data(), ibo_size));

    reset_copy_commands();
    reset_direct_commands();

    copy_buffer(mCopyCommands.get(), vbo, vbo_upload, vbo_size);
    copy_buffer(mCopyCommands.get(), ibo, ibo_upload, ibo_size);

    const D3D12_RESOURCE_BARRIER kBarriers[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(vbo.get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
        CD3DX12_RESOURCE_BARRIER::Transition(ibo.get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_INDEX_BUFFER),
    };

    mCommandList->ResourceBarrier(_countof(kBarriers), kBarriers);

    SM_ASSERT_HR(mCopyCommands->Close());
    SM_ASSERT_HR(mCommandList->Close());

    ID3D12CommandList *copy_lists[] = { mCopyCommands.get() };
    mCopyQueue->ExecuteCommandLists(1, copy_lists);
    SM_ASSERT_HR(mCopyQueue->Signal(*mCopyFence, mCopyFenceValue));

    ID3D12CommandList *direct_lists[] = { mCommandList.get() };
    SM_ASSERT_HR(mDirectQueue->Wait(*mCopyFence, mCopyFenceValue));
    mDirectQueue->ExecuteCommandLists(1, direct_lists);

    wait_for_gpu();

    D3D12_VERTEX_BUFFER_VIEW vbo_view = {
        .BufferLocation = vbo.get_gpu_address(),
        .SizeInBytes = static_cast<uint>(vbo_size),
        .StrideInBytes = sizeof(world::Vertex),
    };

    D3D12_INDEX_BUFFER_VIEW ibo_view = {
        .BufferLocation = ibo.get_gpu_address(),
        .SizeInBytes = static_cast<uint>(ibo_size),
        .Format = DXGI_FORMAT_R16_UINT,
    };

    MeshResource resource = {
        .vbo = std::move(vbo),
        .vbo_view = vbo_view,

        .ibo = std::move(ibo),
        .ibo_view = ibo_view,

        .index_count = int_cast<uint>(mesh.indices.size()),
    };

    mMeshes.emplace(index, std::move(resource));
}

void Context::load_buffer_view(world::IndexOf<world::Model> index, const world::BufferView& view) {

}

void Context::load_object(world::IndexOf<world::Model> index, const world::Object& object) {

}

void Context::create_node(world::IndexOf<world::Node> node) {
    const auto& info = mWorld.get(node);
    for (auto model : info.models) {
        create_model(model);
    }

    for (auto child : info.children) {
        create_node(child);
    }
}

void Context::create_model(world::IndexOf<world::Model> model) {
    if (mMeshes.contains(model)) return;

    const auto& info = mWorld.get(model);

    if (const auto *object = std::get_if<world::Object>(&info.mesh)) {
        // this is a mesh loaded from a buffer or file
        load_object(model, *object);
    } else {
        // this is a primitive mesh
        auto mesh = std::visit([&](auto&& arg) -> world::Mesh {
            if constexpr (!std::is_same_v<std::decay_t<decltype(arg)>, world::Object>)
                return world::primitive(arg);
            else
                CT_NEVER("invalid mesh type");
        }, info.mesh);

        load_mesh_buffer(model, mesh);
    }
}

void Context::begin_upload() {
    wait_for_gpu();
}

void Context::end_upload() {
    // TODO: empty for now
}

void Context::create_scene() {
    for (size_t i = 0; i < mWorld.models.size(); i++) {
        create_model(i);
    }

    for (size_t i = 0; i < mWorld.images.size(); i++) {
        load_image(i);
    }

    create_lights();
}

void Context::destroy_scene() {
    destroy_lights();

    mMeshes.clear();

    for (auto& [_, data] : mImages)
        mSrvPool.release(data.srv);
    mImages.clear();
}

void Context::set_scene(world::IndexOf<world::Scene> scene) {
    if (scene == mCurrentScene) return;

    mCurrentScene = scene;

    wait_for_gpu();
    destroy_scene();
    create_scene();
}

void Context::create_assets() {
    SM_ASSERT_HR(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, *mFrames[mFrameIndex].allocator, nullptr, IID_PPV_ARGS(&mCommandList)));
    SM_ASSERT_HR(mCommandList->Close());

    SM_ASSERT_HR(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
    mFrames[mFrameIndex].fence_value += 1;

    SM_ASSERT_WIN32(mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr));
}

void Context::build_command_list() {
    mAllocator->SetCurrentFrameIndex(mFrameIndex);

    auto& [backbuffer, _, rtv, _] = mFrames[mFrameIndex];

    mFrameGraph.update(mSwapChainHandle, backbuffer.get());
    mFrameGraph.update_rtv(mSwapChainHandle, rtv);

    mFrameGraph.execute();
}

void Context::create_framegraph() {
    graph::ResourceInfo info = {
        .sz = graph::ResourceSize::tex2d(mSwapChainConfig.size),
        .format = mSwapChainConfig.format,
    };
    mSwapChainHandle = mFrameGraph.include("BackBuffer", info, graph::Usage::ePresent, nullptr);

    setup_framegraph(mFrameGraph);

    mFrameGraph.compile();
}

void Context::destroy_framegraph() {
    mFrameGraph.reset();
}

uint64 Context::get_image_footprint(
    world::IndexOf<world::Image> image,
    sm::Span<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints)
{
    const auto& info = mWorld.get(image);
    const auto& data = mImages.at(image);

    D3D12_RESOURCE_DESC desc = data.resource.get()->GetDesc();
    uint64 offset = 0;

    if (!D3DX12GetCopyableFootprints(desc, 0, info.mips, 0, footprints.data(), nullptr, nullptr, &offset))
        CT_NEVER("failed to get image footprints");

    return offset;
}

void Context::destroy_device() {
    destroy_framegraph();
    mFrameGraph.reset_device_data();

    // release frame resources
    for (uint i = 0; i < mSwapChainConfig.length; i++) {
        mFrames[i].target.reset();
        mFrames[i].allocator.reset();
    }

    // release sync objects
    mFence.reset();
    mCopyFence.reset();

    // assets
    destroy_scene();

    destroy_compute_queue();

    // copy commands
    destroy_copy_queue();

    // direct commands
    mDirectQueue.reset();
    mCommandList.reset();

    destroy_device_fence();

    // allocator
    mAllocator.reset();
    mStorage.destroy_queues();

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
    , mSwapChainConfig(config.swapchain)
    , mFrameGraph(*this)
{ }

void Context::create() {
    if (!mInstance.has_viable_adapter()) {
        logs::gRender.error("no viable adapter found, exiting...");
        return;
    }

    Adapter& adapter = [&] -> Adapter& {
        if (auto luid = sm::override_adapter_luid(); luid.has_value()) {
            auto& v = luid.value();
            if (Adapter *adapter = mInstance.get_adapter_by_luid(v)) {
                return *adapter;
            }

            logs::gRender.warn("adapter with luid {:x}:{:x} not found, falling back to default adapter", v.HighPart, v.LowPart);
            return mInstance.get_default_adapter();
        } else if (mDebugFlags.test(DebugFlags::eWarpAdapter)) {
            return mInstance.get_warp_adapter();
        } else {
            return mInstance.get_default_adapter();
        }
    }();

    PIXSetTargetWindow(mConfig.window.get_handle());

    mStorage.create(mDebugFlags);

    create_device(adapter);
    create_device_fence();
    create_allocator();
    create_compute_queue();
    create_copy_queue();
    create_copy_fence();
    create_pipeline();
    on_create();

    create_assets();
    init_scene();
    create_scene();

    create_framegraph();
}

void Context::destroy() {
    flush_copy_queue();
    wait_for_gpu();

    destroy_framegraph();
    on_destroy();

    destroy_dstorage();

    SM_ASSERT_WIN32(CloseHandle(mCopyFenceEvent));
    SM_ASSERT_WIN32(CloseHandle(mFenceEvent));
}

void Context::render() {
    ZoneScopedN("Render");
    build_command_list();

    bool tearing = mInstance.tearing_support();
    uint flags = tearing ? DXGI_PRESENT_ALLOW_TEARING : 0;

    SM_ASSERT_HR(mSwapChain->Present(0, flags));

    FrameMark;

    move_to_next_frame();
}

void Context::update_adapter(Adapter& adapter) {
    if (adapter == *mCurrentAdapter) return;

    recreate_device();
}

void Context::recreate_device() {
    wait_for_gpu();

    on_destroy();
    destroy_device();

    create_device(*mCurrentAdapter);
    create_allocator();
    create_device_fence();
    create_compute_queue();
    create_copy_queue();
    create_copy_fence();

    create_pipeline();
    on_create();

    create_assets();

    create_scene();
    create_framegraph();
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
    SM_ASSERT_HR(mSwapChain->ResizeBuffers(length, mSwapChainConfig.size.width, mSwapChainConfig.size.height, mSwapChainConfig.format, flags));
    mSwapChainConfig.length = length;

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

    for (uint i = 0; i < mSwapChainConfig.length; i++) {
        mFrames[i].target.reset();
        mFrames[i].fence_value = mFrames[mFrameIndex].fence_value;
    }

    destroy_framegraph();
    destroy_frame_rtvs();

    const uint flags = get_swapchain_flags(mInstance);
    SM_ASSERT_HR(mSwapChain->ResizeBuffers(mSwapChainConfig.length, size.width, size.height, mSwapChainConfig.format, flags));
    mSwapChainConfig.size = size;

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

    create_frame_rtvs();
    create_render_targets();
    create_framegraph();
}

void Context::update_framegraph() {
    wait_for_gpu();
    destroy_framegraph();
    create_framegraph();
}

void Context::move_to_next_frame() {
    const uint64 current = mFrames[mFrameIndex].fence_value;
    SM_ASSERT_HR(mDirectQueue->Signal(*mFence, current));

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

    if (mFence->GetCompletedValue() < mFrames[mFrameIndex].fence_value) {
        SM_ASSERT_HR(mFence->SetEventOnCompletion(mFrames[mFrameIndex].fence_value, mFenceEvent));
        PIXNotifyWakeFromFenceSignal(mFenceEvent);
        WaitForSingleObject(mFenceEvent, INFINITE);
    }

    mFrames[mFrameIndex].fence_value = current + 1;
}

void Context::wait_for_gpu() {
    const uint64 current = mFrames[mFrameIndex].fence_value++;
    SM_ASSERT_HR(mDirectQueue->Signal(*mFence, current));

    SM_ASSERT_HR(mFence->SetEventOnCompletion(current, mFenceEvent));
    PIXNotifyWakeFromFenceSignal(mFenceEvent);
    WaitForSingleObject(mFenceEvent, INFINITE);
}

void Context::flush_copy_queue() {
    const uint64 current = mCopyFenceValue++;
    SM_ASSERT_HR(mCopyQueue->Signal(*mCopyFence, current));

    const uint64 completed = mCopyFence->GetCompletedValue();

    if (completed < current) {
        SM_ASSERT_HR(mCopyFence->SetEventOnCompletion(current, mCopyFenceEvent));
        PIXNotifyWakeFromFenceSignal(mCopyFenceEvent);
        WaitForSingleObject(mCopyFenceEvent, INFINITE);
    }
}

/**
#if 0
texindex Context::load_texture(const fs::path& path) {
    Texture tex;
    if (create_texture(tex, path, ImageFormat::eUnknown)) {
        texindex index = int_cast<texindex>(mTextures.size());
        mTextures.emplace_back(std::move(tex));
        return index;
    }

    return UINT16_MAX;
}

texindex Context::load_texture(const ImageData& image) {
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

    texture.rename("temp texture");
    upload.rename("temp texture (upload)");

    D3D12_RANGE read{0, 0};
    void *data;
    SM_ASSERT_HR(upload.map(&read, &data));
    std::memcpy(data, image.data.data(), image.data.size());
    upload.unmap(&read);

    wait_for_gpu();

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
    SM_ASSERT_HR(mDirectQueue->Wait(*mCopyFence, mCopyFenceValue));
    mDirectQueue->ExecuteCommandLists(1, direct_lists);

    wait_for_gpu();
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

    Texture tex = {
        .path = "temp texture",
        .name = "temp texture",
        .format = ImageFormat::eUnknown,
        .size = image.size,
        .mips = 1,

        .resource = std::move(texture),
        .srv = index,
    };

    texindex idx = int_cast<texindex>(mTextures.size());
    mTextures.emplace_back(std::move(tex));
    return idx;
}

void Context::create_world_resources() {
    // TODO:
}

bool Context::create_texture_stb(Texture& result, const fs::path& path) {
    auto image = open_image(path);
    if (image.size == 0u) return false;

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

    texture.rename(path.string());
    upload.rename(fmt::format("{} (upload)", path));

    D3D12_RANGE read{0, 0};
    void *data;
    SM_ASSERT_HR(upload.map(&read, &data));
    std::memcpy(data, image.data.data(), image.data.size());
    upload.unmap(&read);

    wait_for_gpu();

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
    SM_ASSERT_HR(mDirectQueue->Wait(*mCopyFence, mCopyFenceValue));
    mDirectQueue->ExecuteCommandLists(1, direct_lists);

    wait_for_gpu();
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

    result = Texture {
        .path = path,
        .name = path.filename().string(),
        .format = image.format,
        .size = image.size,
        .mips = 1,

        .resource = std::move(texture),
        .srv = index,
    };

    return true;
}

bool Context::create_texture_dds(Texture& result, const fs::path& path) {
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
        logs::gRender.warn("failed to load dds {}: {}", path, hr);
        return false;
    }

    const uint kUploadBufferSize = GetRequiredIntermediateSize(*texture, 0, int_cast<uint>(mips.size()));
    const auto kUploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(kUploadBufferSize);

    Resource upload;

    SM_ASSERT_HR(create_resource(upload, D3D12_HEAP_TYPE_UPLOAD, kUploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ));

    texture.rename(path.string());
    upload.rename(fmt::format("{} (upload)", path));

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
    SM_ASSERT_HR(mDirectQueue->Wait(*mCopyFence, mCopyFenceValue));
    mDirectQueue->ExecuteCommandLists(1, direct_lists);

    wait_for_gpu();
    flush_copy_queue();

    auto desc = texture->GetDesc();

    const D3D12_SHADER_RESOURCE_VIEW_DESC kSrvDesc = {
        .Format = desc.Format,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D = {
            .MostDetailedMip = 0,
            .MipLevels = int_cast<uint>(mips.size()),
            .ResourceMinLODClamp = 0.f,
        },
    };

    SrvIndex index = mSrvPool.allocate();
    auto cpu = mSrvPool.cpu_handle(index);
    mDevice->CreateShaderResourceView(*texture, &kSrvDesc, cpu);

    result = Texture {
        .path = path,
        .name = path.filename().string(),
        .format = ImageFormat::eBC7,
        .size = { int_cast<uint>(desc.Width), int_cast<uint>(desc.Height) },
        .mips = int_cast<uint>(mips.size()),

        .resource = {
            .mResource = std::move(texture),
        },
        .srv = index,
    };

    return true;
}

bool Context::create_texture(Texture& result, const fs::path& path, ImageFormat type) {
    auto str = path.string();
    using enum ImageFormat::Inner;
    switch (type) {
    case eUnknown:
        if (create_texture_stb(result, path))
            return true;
        if (create_texture_dds(result, path))
            return true;
        logs::gRender.warn("unable to determine image type of {}", path);
        break;
    case ePNG:
    case eJPG:
    case eBMP:
        return create_texture_stb(result, path);

    case eBC7:
        return create_texture_dds(result, path);

    default:
        logs::gRender.error("unsupported image type: {}", type);
        return false;
    }

    return false;
}

Mesh Context::create_mesh(const world::MeshInfo& info, const float3& colour) {
    auto mesh = world::primitive(info);

    uint32_t col = math::pack_colour(float4(colour, 1.0f));

    for (auto& v : mesh.vertices) {
        v.colour = col;
    }

    Resource vbo_upload;
    Resource ibo_upload;

    const uint kVertexBufferSize = (uint)mesh.vertices.size() * sizeof(world::Vertex);
    const uint kIndexBufferSize = (uint)mesh.indices.size() * sizeof(uint16);

    const auto kVertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(kVertexBufferSize);
    const auto kIndexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(kIndexBufferSize);

    Mesh primitive = {
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
    SM_ASSERT_HR(mCopyQueue->Signal(*mCopyFence, mCopyFenceValue));

    ID3D12CommandList *direct_lists[] = { mCommandList.get() };
    SM_ASSERT_HR(mDirectQueue->Wait(*mCopyFence, mCopyFenceValue));
    mDirectQueue->ExecuteCommandLists(1, direct_lists);

    primitive.mVertexBufferView = {
        .BufferLocation = primitive.mVertexBuffer.get_gpu_address(),
        .SizeInBytes = kVertexBufferSize,
        .StrideInBytes = sizeof(world::Vertex),
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
#endif
*/
