#include "stdafx.hpp"

#include "config/config.hpp"
#include "render/render.hpp"

using namespace sm;
using namespace sm::render;

#define SM_ASSERT_WIN32(expr)                                  \
    do {                                                       \
        if (auto result = (expr); !result) {                   \
            sm::system::assertLastError(CT_SOURCE_CURRENT, #expr); \
        }                                                      \
    } while (0)

static sm::config::Group gRenderGroup = {
    name = "render",
    desc = "Render configuration options",
};

static sm::opt<std::string> gOverrideLUID = {
    name = "adapter",
    desc = "Override the default adapter LUID",
    group = gRenderGroup
};

static sm::opt<bool> gPixEnabled {
    name = "pix",
    desc = "Enable PIX debugging",
    init = false,
    group = gRenderGroup
};

static sm::opt<bool> gWarpEnabled {
    name = "warp",
    desc = "Enable WARP adapter",
    init = false,
    group = gRenderGroup
};

static sm::opt<bool> gDredEnabled {
    name = "dred",
    desc = "Enable DRED debugging",
    init = false,
    group = gRenderGroup
};

static sm::opt<bool> gDebugEnabled {
    name = "d3d12-debug-layer",
    desc = "Enable d3d12 debug layer",
    init = false,
    group = gRenderGroup
};

static render::DebugFlags getDebugFlags() {
    render::DebugFlags flags = render::DebugFlags::eNone;

    if (gWarpEnabled.getValue()) {
        flags |= render::DebugFlags::eWarpAdapter;
    }

    if (gDebugEnabled.getValue()) {
        flags |= render::DebugFlags::eDeviceDebugLayer;
        flags |= render::DebugFlags::eFactoryDebug;
        flags |= render::DebugFlags::eInfoQueue;
        flags |= render::DebugFlags::eAutoName;
        flags |= render::DebugFlags::eDirectStorageDebug;

        // enabling gpu based validation on the warp adapter
        // tanks performance
        if (!gWarpEnabled.getValue()) {
            flags |= render::DebugFlags::eGpuValidation;
        }
    }

    if (gPixEnabled.getValue()) {
        flags |= render::DebugFlags::eWinPixEventRuntime;
    }

    if (gDredEnabled.getValue()) {
        flags |= render::DebugFlags::eDeviceRemovedInfo;
    }

    return flags;
}

static LUID getOverrideLUID() {
    CTASSERTF(gOverrideLUID.isModified(), "adapter LUID is not set");

    auto [high, low] = sm::split(gOverrideLUID.getValue(), ':');
    LUID result { };

    {
        auto [ptr, ec] = std::from_chars(high.data(), high.data() + high.size(), result.HighPart, 16);
        if (ec != std::errc()) {
            LOG_ERROR(GpuLog, "failed to parse high part of adapter LUID: {}", high);
            return { 0, 0 };
        }
    }

    {
        auto [ptr, ec] = std::from_chars(low.data(), low.data() + low.size(), result.LowPart, 16);
        if (ec != std::errc()) {
            LOG_ERROR(GpuLog, "failed to parse low part of adapter LUID: {}", low);
            return { 0, 0 };
        }
    }

    return result;
}

#pragma region Storage queue creation and lifetime

void IDeviceContext::createStorageContext() {
    mStorage.create(mDebugFlags);
}

void IDeviceContext::destroyStorageContext() {
    mStorage.destroy();
}

void IDeviceContext::createStorageDeviceData() {
    // create a fence and event for this storage context
    SM_ASSERT_HR(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mStorageFence)));
    mStorageFenceValue = 1;

    mStorageFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    SM_ASSERT(mStorageFenceEvent != nullptr);

    // now create storage queues
    mMemoryQueue = mStorage.newQueue({
        .SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY,
        .Capacity = DSTORAGE_MAX_QUEUE_CAPACITY,
        .Priority = DSTORAGE_PRIORITY_NORMAL,
        .Name = "host -> device",
        .Device = mDevice.get(),
    });

    mFileQueue = mStorage.newQueue({
        .SourceType = DSTORAGE_REQUEST_SOURCE_FILE,
        .Capacity = DSTORAGE_MAX_QUEUE_CAPACITY,
        .Priority = DSTORAGE_PRIORITY_NORMAL,
        .Name = "disk -> device",
        .Device = mDevice.get(),
    });
}

void IDeviceContext::destroyStorageDeviceData() {
    // destroy our queues first
    // TODO: should we ensure they're empty?
    mMemoryQueue.reset();
    mFileQueue.reset();

    // destroy the fence and handle
    SM_ASSERT_WIN32(CloseHandle(mStorageFenceEvent));
    mStorageFence.reset();
}

IDStorageFile *IDeviceContext::getStorageFile(world::IndexOf<world::File> index) {
    if (auto it = mStorageFiles.find(index); it != mStorageFiles.end()) {
        return it->second.get();
    }

    const auto& file = mWorld.get(index);

    Object<IDStorageFile> dsfile;
    SM_ASSERT_HR(mStorage.open(file.path, &dsfile));

    auto [it, _] = mStorageFiles.emplace(index, std::move(dsfile));
    auto& [_, fd] = *it;

    return fd.get();
}

const uint8 *IDeviceContext::getStorageBuffer(world::IndexOf<world::Buffer> index) {
    auto& buffer = mWorld.get(index);

    return buffer.data.data();
}

#pragma region Device creation and lifetime

static uint getSwapChainFlags(const Instance& instance) {
    return instance.isTearingSupported() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
}

static void onQueueMessage(D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID id, LPCSTR desc, void *user) {
    MessageCategory c{category};
    MessageSeverity s{severity};
    MessageID message{id};

    switch (severity) {
    case D3D12_MESSAGE_SEVERITY_CORRUPTION:
        LOG_PANIC(GpuLog, "{} {}: {}", c, message, desc);
        break;
    case D3D12_MESSAGE_SEVERITY_ERROR:
        LOG_ERROR(GpuLog, "{} {}: {}", c, message, desc);
        break;
    case D3D12_MESSAGE_SEVERITY_WARNING:
        LOG_WARN(GpuLog, "{} {}: {}", c, message, desc);
        break;
    case D3D12_MESSAGE_SEVERITY_INFO:
        LOG_INFO(GpuLog, "{} {}: {}", c, message, desc);
        break;
    case D3D12_MESSAGE_SEVERITY_MESSAGE:
        LOG_DEBUG(GpuLog, "{} {}: {}", c, message, desc);
        break;
    }
}

void IDeviceContext::enable_info_queue() {
    if (Result hr = mDevice.query(&mInfoQueue)) {
        mInfoQueue->RegisterMessageCallback(&onQueueMessage, D3D12_MESSAGE_CALLBACK_FLAG_NONE, this, &mCookie);
        mInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        mInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
    } else {
        LOG_WARN(GpuLog, "failed to query info queue: {}", hr);
    }
}

void IDeviceContext::enable_debug_layer(bool gbv, bool rename) {
    if (Result hr = D3D12GetDebugInterface(IID_PPV_ARGS(&mDebug))) {
        mDebug->EnableDebugLayer();

        Object<ID3D12Debug3> debug3;
        if (mDebug.query(&debug3)) debug3->SetEnableGPUBasedValidation(gbv);
        else LOG_WARN(GpuLog, "failed to get debug3 interface: {}", hr);

        Object<ID3D12Debug5> debug5;
        if (mDebug.query(&debug5)) debug5->SetEnableAutoName(rename);
        else LOG_WARN(GpuLog, "failed to get debug5 interface: {}", hr);
    } else {
        LOG_WARN(GpuLog, "failed to get debug interface: {}", hr);
    }
}

void IDeviceContext::disable_debug_layer() {
    Object<ID3D12Debug4> debug4;
    if (Result hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debug4))) {
        debug4->DisableDebugLayer();
    } else {
        LOG_WARN(GpuLog, "failed to query debug4 interface: {}", hr);
    }
}

void IDeviceContext::enable_dred(bool enabled) {
    Object<ID3D12DeviceRemovedExtendedDataSettings> dred;
    if (Result hr = D3D12GetDebugInterface(IID_PPV_ARGS(&dred))) {
        dred->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        dred->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        LOG_INFO(GpuLog, "dred enabled");
    } else {
        LOG_WARN(GpuLog, "failed to query dred settings: {}", hr);
    }
}

void IDeviceContext::query_root_signature_version() {
    mRootSignatureVersion = mFeatureSupport.HighestRootSignatureVersion();
    LOG_INFO(GpuLog, "root signature version: {}", mRootSignatureVersion);
}

void IDeviceContext::serialize_root_signature(Object<ID3D12RootSignature>& signature, const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc) {
    Blob serialized;
    Blob error;
    if (Result hr = D3DX12SerializeVersionedRootSignature(&desc, mRootSignatureVersion.as_facade(), &serialized, &error); !hr) {
        LOG_ERROR(GpuLog, "failed to serialize root signature: {}", hr);
        LOG_ERROR(GpuLog, "| error: {}", error.asString());
        SM_ASSERT_HR(hr);
    }

    SM_ASSERT_HR(mDevice->CreateRootSignature(0, serialized.data(), serialized.size(), IID_PPV_ARGS(&signature)));
}

void IDeviceContext::create_device(Adapter& adapter) {
    if (auto flags = adapter.flags(); flags.test(AdapterFlag::eSoftware) && bool(mDebugFlags & DebugFlags::eGpuValidation)) {
        LOG_WARN(GpuLog, "adapter `{}` is a software adapter, enabling gpu validation has major performance implications", adapter.name());
    }

    if (bool(mDebugFlags & DebugFlags::eDeviceDebugLayer))
        enable_debug_layer(bool(mDebugFlags & DebugFlags::eGpuValidation), bool(mDebugFlags & DebugFlags::eAutoName));
    else
        disable_debug_layer();

    enable_dred(bool(mDebugFlags & DebugFlags::eDeviceRemovedInfo));

    FeatureLevel fl = getMinFeatureLevel();

    set_current_adapter(adapter);
    if (Result hr = D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL(fl), IID_PPV_ARGS(&mDevice)); !hr) {
        LOG_ERROR(GpuLog, "failed to create device `{}` at feature level `{}`", adapter.name(), fl, hr);
        LOG_ERROR(GpuLog, "| hresult: {}", hr);
        LOG_ERROR(GpuLog, "falling back to warp adapter...");

        Adapter& warp = mInstance.getWarpAdapter();
        set_current_adapter(warp);
        SM_ASSERT_HR(D3D12CreateDevice(warp.get(), D3D_FEATURE_LEVEL(fl), IID_PPV_ARGS(&mDevice)));
    }

    LOG_INFO(GpuLog, "device created: {}", mCurrentAdapter->name());

    if (bool(mDebugFlags & DebugFlags::eInfoQueue))
        enable_info_queue();

    SM_ASSERT_HR(mFeatureSupport.Init(*mDevice));

    query_root_signature_version();

    LOG_INFO(GpuLog, "| feature level: {}", fl);
    LOG_INFO(GpuLog, "| flags: {}", mDebugFlags);
}

static constexpr D3D12MA::ALLOCATOR_FLAGS kAllocatorFlags = D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED | D3D12MA::ALLOCATOR_FLAG_SINGLETHREADED;

void IDeviceContext::create_allocator() {
    const D3D12MA::ALLOCATOR_DESC kAllocatorDesc = {
        .Flags = kAllocatorFlags,
        .pDevice = mDevice.get(),
        .pAdapter = mCurrentAdapter->get()
    };

    SM_ASSERT_HR(D3D12MA::CreateAllocator(&kAllocatorDesc, &mAllocator));
}

void IDeviceContext::reset_direct_commands(ID3D12PipelineState *pso) {
    auto& allocator = mFrames[mFrameIndex].allocator;
    SM_ASSERT_HR(allocator->Reset());
    SM_ASSERT_HR(mCommandList->Reset(*allocator, pso));
}

void IDeviceContext::createComputeQueue() {
    constexpr D3D12_COMMAND_QUEUE_DESC kQueueDesc = {
        .Type = D3D12_COMMAND_LIST_TYPE_COMPUTE,
    };

    SM_ASSERT_HR(mDevice->CreateCommandQueue(&kQueueDesc, IID_PPV_ARGS(&mComputeQueue)));
}

void IDeviceContext::destroyComputeQueue() {
    mComputeQueue.reset();
}

void IDeviceContext::createCopyQueue() {
    constexpr D3D12_COMMAND_QUEUE_DESC kQueueDesc = {
        .Type = D3D12_COMMAND_LIST_TYPE_COPY,
    };

    SM_ASSERT_HR(mDevice->CreateCommandQueue(&kQueueDesc, IID_PPV_ARGS(&mCopyQueue)));
}

void IDeviceContext::destroyCopyQueue() {
    mCopyQueue.reset();
}

ID3D12CommandQueue *IDeviceContext::getQueue(CommandListType type) {
    switch (type) {
    case CommandListType::eDirect: return mDirectQueue.get();
    case CommandListType::eCompute: return mComputeQueue.get();
    case CommandListType::eCopy: return mCopyQueue.get();

    default: CT_NEVER("invalid command list type");
    }
}

void IDeviceContext::create_pipeline() {
    constexpr D3D12_COMMAND_QUEUE_DESC kQueueDesc = {
        .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
    };

    SM_ASSERT_HR(mDevice->CreateCommandQueue(&kQueueDesc, IID_PPV_ARGS(&mDirectQueue)));

    bool tearing = mInstance.isTearingSupported();
    const DXGI_SWAP_CHAIN_DESC1 kSwapChainDesc = {
        .Width = mSwapChainConfig.size.width,
        .Height = mSwapChainConfig.size.height,
        .Format = mSwapChainConfig.format,
        .SampleDesc = { 1, 0 },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = mSwapChainConfig.length,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .Flags = getSwapChainFlags(mInstance),
    };

    IDXGIFactory4 *factory = mInstance.factory();
    HWND hwnd = mConfig.window.getHandle();

    Object<IDXGISwapChain1> swapchain1;
    SM_ASSERT_HR(factory->CreateSwapChainForHwnd(*mDirectQueue, hwnd, &kSwapChainDesc, nullptr, nullptr, &swapchain1));

    if (tearing) {
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
        mFrames[i].fenceValue = 0;
    }
}

static const D3D12_HEAP_PROPERTIES kDefaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
static const D3D12_HEAP_PROPERTIES kUploadHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

void IDeviceContext::create_frame_rtvs() {
    for (auto& frame : mFrames) {
        frame.rtvIndex = mRtvPool.allocate();
    }
}

void IDeviceContext::destroy_frame_rtvs() {
    for (auto& frame : mFrames) {
        mRtvPool.release(frame.rtvIndex);
        frame.rtvIndex = RtvIndex::eInvalid;
    }
}

void IDeviceContext::create_render_targets() {
    CTASSERTF(mSwapChainConfig.length <= DXGI_MAX_SWAP_CHAIN_BUFFERS, "too many swap chain buffers (%u > %u)", mSwapChainConfig.length, DXGI_MAX_SWAP_CHAIN_BUFFERS);

    for (uint i = 0; i < mSwapChainConfig.length; i++) {
        auto& backbuffer = mFrames[i].target;
        auto rtv = mRtvPool.cpu_handle(mFrames[i].rtvIndex);
        SM_ASSERT_HR(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&backbuffer)));
        mDevice->CreateRenderTargetView(*backbuffer, nullptr, rtv);

        backbuffer.rename(fmt::format("BackBuffer {}", i));
    }
}

void IDeviceContext::create_frame_allocators() {
    for (uint i = 0; i < mSwapChainConfig.length; i++) {
        SM_ASSERT_HR(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mFrames[i].allocator)));
    }
}

Result IDeviceContext::createTextureResource(Resource& resource, D3D12_HEAP_TYPE heap, D3D12_RESOURCE_DESC desc, D3D12_RESOURCE_STATES state, const D3D12_CLEAR_VALUE *clear) {
    const D3D12MA::ALLOCATION_DESC kAllocDesc = {
        .HeapType = heap,
    };

    Result hr = mAllocator->CreateResource(&kAllocDesc, &desc, state, clear, &resource.mAllocation, IID_PPV_ARGS(&resource.mResource));
    if (hr.failed())
        return hr;

    return S_OK;
}

Result IDeviceContext::createBufferResource(
    Resource& resource,
    D3D12_HEAP_TYPE heap,
    uint64 width,
    D3D12_RESOURCE_STATES state,
    D3D12_RESOURCE_FLAGS flags
)
{
    const D3D12MA::ALLOCATION_DESC kAllocDesc = {
        .HeapType = heap,
    };

    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(width, flags);

    Result hr = mAllocator->CreateResource(&kAllocDesc, &desc, state, nullptr, &resource.mAllocation, IID_PPV_ARGS(&resource.mResource));
    if (hr.failed())
        return hr;

    resource.mGpuAddress = resource.mResource->GetGPUVirtualAddress();

    return S_OK;
}

void IDeviceContext::init_scene() {
    mWorld = world::default_world("Default World");
    mCurrentScene = mWorld.default_scene;
}

static void buildFileUploadRequest(
    render::IDeviceContext& self,
    render::RequestBuilder& request,
    world::IndexOf<world::File> file,
    uint64 offset,
    uint32 size)
{
    IDStorageFile *storage = self.getStorageFile(file);

    self.mFileQueue.enqueue(request.src(storage, offset, size).name("Load File Region"));
}

static void buildBufferUploadRequest(
    render::IDeviceContext& self,
    render::RequestBuilder& request,
    world::IndexOf<world::Buffer> buffer,
    uint64 offset,
    uint32 size)
{
    const uint8 *data = self.getStorageBuffer(buffer) + offset;

    self.mMemoryQueue.enqueue(request.src(data, size).name("Load Buffer Region"));
}

static void buildUploadRequest(IDeviceContext& self, render::RequestBuilder& request, const world::BufferView& view) {
    if (world::IndexOf file = world::get<world::File>(view.source); file != world::kInvalidIndex) {
        buildFileUploadRequest(self, request, file, view.offset, view.source_size);
    }
    else if (world::IndexOf buffer = world::get<world::Buffer>(view.source); buffer != world::kInvalidIndex) {
        buildBufferUploadRequest(self, request, buffer, view.offset, view.source_size);
    }
    else {
        CT_NEVER("invalid buffer source");
    }
}

static Resource addBufferViewUpload(IDeviceContext& self, const world::BufferView& view) {
    Resource resource;
    SM_ASSERT_HR(self.createBufferResource(resource, D3D12_HEAP_TYPE_DEFAULT, view.source_size, D3D12_RESOURCE_STATE_COMMON));

    render::RequestBuilder request{};
    request.dst(resource.get(), 0, view.source_size);

    buildUploadRequest(self, request, view);

    return resource;
}

void IDeviceContext::upload_buffer_view(RequestBuilder& request, const world::BufferView& view) {
    if (world::IndexOf file = world::get<world::File>(view.source); file != world::kInvalidIndex) {
        request.src(getStorageFile(file), view.offset, view.source_size);
        mFileQueue.enqueue(request);
    }
    else if (world::IndexOf buffer = world::get<world::Buffer>(view.source); buffer != world::kInvalidIndex) {
        request.src(getStorageBuffer(buffer) + view.offset, view.source_size);
        mMemoryQueue.enqueue(request);
    }
    else {
        CT_NEVER("invalid image source");
    }
}

void IDeviceContext::upload_image(world::IndexOf<world::Image> index) {
    const auto& image = mWorld.get(index);

    Resource data;
    auto desc = CD3DX12_RESOURCE_DESC::Tex2D(image.format, image.size.width, image.size.height, 1, image.mips);
    SM_ASSERT_HR(createTextureResource(data, D3D12_HEAP_TYPE_DEFAULT, desc, D3D12_RESOURCE_STATE_COPY_DEST));

    RequestBuilder request;
    request.dst(DSTORAGE_DESTINATION_MULTIPLE_SUBRESOURCES{ data.get(), 0 });

    buildUploadRequest(*this, request, image.source);

    mPendingBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(data.get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

    SrvIndex srv = mSrvPool.allocate();
    auto cpu = mSrvPool.cpu_handle(srv);

    mDevice->CreateShaderResourceView(data.get(), nullptr, cpu);

    TextureResource resource = {
        .resource = std::move(data),
        .srv = srv,
    };

    mImages.emplace(index, std::move(resource));
}

void IDeviceContext::load_mesh_buffer(world::IndexOf<world::Model> index, world::Mesh&& mesh) {
    render::ecs::VertexBuffer vbo = uploadVertexBuffer(std::move(mesh.vertices));
    render::ecs::IndexBuffer ibo = uploadIndexBuffer(std::move(mesh.indices));

    MeshResource resource = {
        .vbo = std::move(vbo),
        .ibo = std::move(ibo),
    };

    mMeshes.emplace(index, std::move(resource));
}

void IDeviceContext::upload_object(world::IndexOf<world::Model> index, const world::Object& object) {
    Resource vbo = addBufferViewUpload(*this, object.vertices);
    Resource ibo = addBufferViewUpload(*this, object.indices);

    mPendingBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(vbo.get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
    mPendingBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(ibo.get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_INDEX_BUFFER));

    D3D12_VERTEX_BUFFER_VIEW vbo_view = {
        .BufferLocation = vbo.getDeviceAddress(),
        .SizeInBytes = object.vertices.getBufferSize(),
        .StrideInBytes = object.getVertexStride(),
    };

    D3D12_INDEX_BUFFER_VIEW ibo_view = {
        .BufferLocation = ibo.getDeviceAddress(),
        .SizeInBytes = object.indices.getBufferSize(),
        .Format = object.getIndexBufferFormat(),
    };

    ecs::VertexBuffer vertices = {
        .resource = std::move(vbo),
        .view = vbo_view,
        .stride = object.getVertexStride(),
        .length = object.getVertexCount(),
    };

    ecs::IndexBuffer indices = {
        .resource = std::move(ibo),
        .view = ibo_view,
        .format = object.getIndexBufferFormat(),
        .length = object.getIndexCount(),
    };

    MeshResource resource = {
        .vbo = std::move(vertices),
        .ibo = std::move(indices),

        .bounds = world::computeObjectBounds(mWorld, object),
    };

    mMeshes.emplace(index, std::move(resource));
}

void IDeviceContext::create_node(world::IndexOf<world::Node> node) {
    const auto& info = mWorld.get(node);
    for (auto model : info.models) {
        upload_model(model);
    }

    for (auto child : info.children) {
        create_node(child);
    }
}

template<typename... T>
struct overloaded : T... {
    using T::operator()...;
};

void IDeviceContext::upload_model(world::IndexOf<world::Model> model) {
    if (mMeshes.contains(model)) return;

    const auto& info = mWorld.get(model);

    if (const auto *object = std::get_if<world::Object>(&info.mesh)) {
        // this is a mesh loaded from a buffer or file
        upload_object(model, *object);
    }
    else {
        // this is a primitive mesh
        auto mesh = std::visit([&](auto&& arg) -> world::Mesh {
            if constexpr (!std::is_same_v<std::decay_t<decltype(arg)>, world::Object>) {
                return world::primitive(arg);
            } else {
                CT_NEVER("invalid mesh type");
            }
        }, info.mesh);

        load_mesh_buffer(model, std::move(mesh));
    }
}

render::ecs::VertexBuffer IDeviceContext::uploadVertexBuffer(world::VertexBuffer&& buffer) {
    uint length = uint(buffer.size());
    size_t size = buffer.sizeInBytes();

    Resource resource;
    SM_ASSERT_HR(createBufferResource(resource, D3D12_HEAP_TYPE_DEFAULT, size, D3D12_RESOURCE_STATE_COMMON));

    mPendingBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(resource.get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

    render::RequestBuilder request = render::RequestBuilder()
        .src(buffer.data(), size)
        .name("VertexBuffer")
        .dst(resource.get(), 0, size);

    mMemoryQueue.enqueue(request);
    mCopyData.emplace(HostCopySource{ mStorageFenceValue, std::move(buffer) });

    D3D12_VERTEX_BUFFER_VIEW view = {
        .BufferLocation = resource.getDeviceAddress(),
        .SizeInBytes = uint32(size),
        .StrideInBytes = sizeof(world::Vertex)
    };

    return { std::move(resource), view, sizeof(world::Vertex), length };
}

render::ecs::IndexBuffer IDeviceContext::uploadIndexBuffer(world::IndexBuffer&& buffer) {
    uint length = uint(buffer.size());
    size_t size = buffer.sizeInBytes();

    Resource resource;
    SM_ASSERT_HR(createBufferResource(resource, D3D12_HEAP_TYPE_DEFAULT, size, D3D12_RESOURCE_STATE_COMMON));

    mPendingBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(resource.get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_INDEX_BUFFER));

    render::RequestBuilder request = render::RequestBuilder()
        .src(buffer.data(), size)
        .name("IndexBuffer")
        .dst(resource.get(), 0, size);

    mMemoryQueue.enqueue(request);
    mCopyData.emplace(HostCopySource{ mStorageFenceValue, std::move(buffer) });

    D3D12_INDEX_BUFFER_VIEW view = {
        .BufferLocation = resource.getDeviceAddress(),
        .SizeInBytes = uint32(size),
        .Format = DXGI_FORMAT_R16_UINT
    };

    return { std::move(resource), view, DXGI_FORMAT_R16_UINT, length };
}

void IDeviceContext::begin_upload() {
    wait_for_gpu();
    mPendingBarriers.clear();
}

void IDeviceContext::end_upload() {
    bool hasWork = mFileQueue.hasPendingRequests() || mMemoryQueue.hasPendingRequests() || !mPendingBarriers.empty();
    if (!hasWork) return;

    // only signal and submit when the queues have pending requests
    // dstorage doesnt signal a fence if there are no prior requests
    if (mFileQueue.hasPendingRequests()) {
        mFileQueue.signal(*mStorageFence, mStorageFenceValue);
        mFileQueue.submit();

        mDirectQueue->Wait(*mStorageFence, mStorageFenceValue);

        mStorageFenceValue += 1;
    }

    if (mMemoryQueue.hasPendingRequests()) {
        mMemoryQueue.signal(*mStorageFence, mStorageFenceValue);
        mMemoryQueue.submit();

        mDirectQueue->Wait(*mStorageFence, mStorageFenceValue);

        mStorageFenceValue += 1;
    }

    // only submit barriers if there are any
    if (!mPendingBarriers.empty()) {
        reset_direct_commands();

        mCommandList->ResourceBarrier(sm::int_cast<uint>(mPendingBarriers.size()), mPendingBarriers.data());

        SM_ASSERT_HR(mCommandList->Close());

        ID3D12CommandList *lists[] = { mCommandList.get() };

        mDirectQueue->ExecuteCommandLists(1, lists);
        mDirectQueue->Signal(*mStorageFence, mStorageFenceValue++);
    }

    // only wait on the cpu side if commands were submitted
    uint64 expected = mStorageFenceValue - 1;
    uint64 completed = mStorageFence->GetCompletedValue();
    if (completed < expected) {
        mStorageFence->SetEventOnCompletion(expected, mStorageFenceEvent);
        WaitForSingleObject(mStorageFenceEvent, INFINITE);
    }

    // remove everything from the copy data queue thats been processed
    while (!mCopyData.is_empty() && mCopyData.top().fence <= completed) {
        mCopyData.pop();
    }
}

void IDeviceContext::create_scene() {
    upload([&] {
        for (world::IndexOf i : mWorld.indices<world::Model>()) {
            upload_model(i);
        }

        for (world::IndexOf i : mWorld.indices<world::Image>()) {
            upload_image(i);
        }
    });
}

void IDeviceContext::destroy_scene() {
    mMeshes.clear();

    for (auto& [_, data] : mImages)
        mSrvPool.release(data.srv);
    mImages.clear();
}

void IDeviceContext::set_scene(world::IndexOf<world::Scene> scene) {
    if (scene == mCurrentScene) return;

    mCurrentScene = scene;

    wait_for_gpu();
    destroy_scene();
    create_scene();
}

void IDeviceContext::create_assets() {
    SM_ASSERT_HR(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, *mFrames[mFrameIndex].allocator, nullptr, IID_PPV_ARGS(&mCommandList)));
    SM_ASSERT_HR(mCommandList->Close());

    SM_ASSERT_HR(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
    mFrames[mFrameIndex].fenceValue += 1;

    SM_ASSERT_WIN32(mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr));
}

void IDeviceContext::build_command_list() {
    mFrameGraph.setFrameIndex(mFrameIndex);
    mAllocator->SetCurrentFrameIndex(mFrameIndex);

    auto& [backbuffer, _, rtv, _] = mFrames[mFrameIndex];

    mFrameGraph.update(mSwapChainHandle, backbuffer.get());
    mFrameGraph.update_rtv(mSwapChainHandle, rtv);

    mFrameGraph.execute();
}

void IDeviceContext::create_framegraph() {
    graph::ResourceInfo info = graph::ResourceInfo::tex2d(mSwapChainConfig.size, mSwapChainConfig.format);
    mSwapChainHandle = mFrameGraph.include("BackBuffer", info, graph::Usage::ePresent, nullptr);

    setup_framegraph(mFrameGraph);

    mFrameGraph.compile();
}

void IDeviceContext::destroy_framegraph() {
    mFrameGraph.reset();
}

uint64 IDeviceContext::get_image_footprint(
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

void IDeviceContext::destroy_device() {
    destroy_framegraph();
    mFrameGraph.reset_device_data();

    // release frame resources
    for (uint i = 0; i < mSwapChainConfig.length; i++) {
        mFrames[i].target.reset();
        mFrames[i].allocator.reset();
    }

    // release sync objects
    mFence.reset();

    // storage
    destroyStorageDeviceData();

    // assets
    destroy_scene();

    destroyComputeQueue();

    // copy commands
    destroyCopyQueue();

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

IDeviceContext::IDeviceContext(const RenderConfig& config)
    : mConfig(config)
    , mDebugFlags(config.flags | getDebugFlags())
    , mInstance({ mDebugFlags, config.preference })
    , mSwapChainConfig(config.swapchain)
    , mFrameGraph(*this)
{ }

void IDeviceContext::create() {
    if (!mInstance.hasViableAdapter()) {
        LOG_ERROR(GpuLog, "no viable adapter found, exiting...");
        return;
    }

    Adapter& adapter = [&] -> Adapter& {
        if (gOverrideLUID.isModified()) {
            LUID luid = getOverrideLUID();
            if (Adapter *adapter = mInstance.getAdapterByLUID(luid)) {
                return *adapter;
            }
            LOG_WARN(GpuLog, "adapter with luid {:x}:{:x} not found, falling back to default adapter", luid.HighPart, luid.LowPart);
        } else if (bool(mDebugFlags & DebugFlags::eWarpAdapter)) {
            return mInstance.getWarpAdapter();
        }

        return mInstance.getDefaultAdapter();
    }();

    PIXSetTargetWindow(mConfig.window.getHandle());

    createStorageContext();

    create_device(adapter);
    createStorageDeviceData();

    create_allocator();
    createComputeQueue();
    createCopyQueue();
    create_pipeline();

    on_setup();
    on_create();

    create_assets();
    init_scene();
    create_scene();

    create_framegraph();
}

void IDeviceContext::destroy() {
    wait_for_gpu();

    destroy_framegraph();
    on_destroy();

    destroyStorageDeviceData();
    destroyStorageContext();

    SM_ASSERT_WIN32(CloseHandle(mFenceEvent));
}

void IDeviceContext::render() {
    ZoneScopedN("Render");
    build_command_list();

    bool tearing = mInstance.isTearingSupported();
    uint flags = tearing ? DXGI_PRESENT_ALLOW_TEARING : 0;

    SM_ASSERT_HR(mSwapChain->Present(0, flags));

    FrameMark;

    move_to_next_frame();
}

void IDeviceContext::update_adapter(Adapter& adapter) {
    if (adapter == *mCurrentAdapter) return;

    recreate_device();
}

void IDeviceContext::recreate_device() {
    wait_for_gpu();

    on_destroy();
    destroy_device();

    create_device(*mCurrentAdapter);
    create_allocator();
    createComputeQueue();
    createCopyQueue();

    createStorageDeviceData();

    create_pipeline();
    on_create();

    create_assets();

    create_scene();
    create_framegraph();
}

void IDeviceContext::update_swapchain_length(uint length) {
    wait_for_gpu();

    uint64 current = mFrames[mFrameIndex].fenceValue;

    for (auto& frame : mFrames) {
        frame.target.reset();
        frame.allocator.reset();
    }

    destroy_frame_rtvs();

    const uint flags = getSwapChainFlags(mInstance);
    auto [w, h] = getSwapChainSize();
    SM_ASSERT_HR(mSwapChain->ResizeBuffers(length, w, h, mSwapChainConfig.format, flags));
    mSwapChainConfig.length = length;

    mFrames.resize(length);
    for (uint i = 0; i < length; i++) {
        mFrames[i].fenceValue = current;
    }

    create_frame_rtvs();
    create_render_targets();
    create_frame_allocators();
    mFrameGraph.resize_frame_data(length);

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
}

void IDeviceContext::resize_swapchain(math::uint2 size) {
    wait_for_gpu();

    for (uint i = 0; i < mSwapChainConfig.length; i++) {
        mFrames[i].target.reset();
        mFrames[i].fenceValue = mFrames[mFrameIndex].fenceValue;
    }

    destroy_framegraph();
    destroy_frame_rtvs();

    const uint flags = getSwapChainFlags(mInstance);
    SM_ASSERT_HR(mSwapChain->ResizeBuffers(mSwapChainConfig.length, size.width, size.height, mSwapChainConfig.format, flags));
    mSwapChainConfig.size = size;

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

    create_frame_rtvs();
    create_render_targets();
    create_framegraph();
}

void IDeviceContext::update_framegraph() {
    wait_for_gpu();
    destroy_framegraph();
    create_framegraph();
}

void IDeviceContext::move_to_next_frame() {
    const uint64 current = mFrames[mFrameIndex].fenceValue;
    SM_ASSERT_HR(mDirectQueue->Signal(*mFence, current));

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

    if (mFence->GetCompletedValue() < mFrames[mFrameIndex].fenceValue) {
        SM_ASSERT_HR(mFence->SetEventOnCompletion(mFrames[mFrameIndex].fenceValue, mFenceEvent));
        PIXNotifyWakeFromFenceSignal(mFenceEvent);
        WaitForSingleObject(mFenceEvent, INFINITE);
    }

    mFrames[mFrameIndex].fenceValue = current + 1;
}

void IDeviceContext::wait_for_gpu() {
    const uint64 current = mFrames[mFrameIndex].fenceValue++;
    SM_ASSERT_HR(mDirectQueue->Signal(*mFence, current));

    SM_ASSERT_HR(mFence->SetEventOnCompletion(current, mFenceEvent));
    PIXNotifyWakeFromFenceSignal(mFenceEvent);
    WaitForSingleObject(mFenceEvent, INFINITE);
}

void render::copyBufferRegion(ID3D12GraphicsCommandList1 *list, Resource& dst, Resource& src, size_t size) {
    list->CopyBufferRegion(dst.get(), 0, src.get(), 0, size);
}
