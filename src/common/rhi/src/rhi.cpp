#include "rhi/rhi.hpp"
#include "base/panic.h"
#include "core/arena.hpp"
#include "io/io.h"
#include "os/os.h"

#include "d3dx12/d3dx12_barriers.h"
#include "d3dx12/d3dx12_core.h"
#include "d3dx12/d3dx12_root_signature.h"
#include "d3dx12/d3dx12_resource_helpers.h"

using namespace sm;
using namespace sm::rhi;

#define DLLEXPORT __declspec(dllexport)

extern "C" {
    DLLEXPORT DWORD NvOptimusEnablement = 0x00000001;                  // NOLINT
    DLLEXPORT DWORD AmdPowerXpressRequestHighPerformance = 0x00000001; // NOLINT
}

#if SMC_AGILITY_SDK
extern "C" {
    // load the agility sdk
    DLLEXPORT extern const UINT D3D12SDKVersion = 611;        // NOLINT
    DLLEXPORT extern const char *D3D12SDKPath = ".\\D3D12\\"; // NOLINT
}
#endif

static char *hr_string(HRESULT hr) {
    sm::IArena& arena = sm::get_pool(sm::Pool::eDebug);
    return os_error_string(hr, &arena);
}

NORETURN
static assert_hresult(source_info_t panic, const char *expr, HRESULT hr) {
    ctu_panic(panic, "hresult: %s %s", hr_string(hr), expr);
}

logs::Severity MessageSeverity::get_log_severity() const {
    using enum MessageSeverity::Inner;
    switch (m_value) {
    case eCorruption: return logs::Severity::ePanic;
    case eError: return logs::Severity::eError;
    case eWarning: return logs::Severity::eWarning;
    case eInfo: return logs::Severity::eInfo;
    case eMessage:
    default: return logs::Severity::eTrace;
    }
}

D3D12_RESOURCE_STATES ResourceState::get_inner_state() const {
    using enum ResourceState::Inner;
    switch (m_value) {
    case ePresent: return D3D12_RESOURCE_STATE_PRESENT;
    case eRenderTarget: return D3D12_RESOURCE_STATE_RENDER_TARGET;

    case eIndexBuffer: return D3D12_RESOURCE_STATE_INDEX_BUFFER;

    case eVertexBuffer:
    case eConstBuffer:
        return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

    case eCopySource: return D3D12_RESOURCE_STATE_COPY_SOURCE;
    case eCopyTarget: return D3D12_RESOURCE_STATE_COPY_DEST;

    case eDepthWrite: return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    case eDepthRead: return D3D12_RESOURCE_STATE_DEPTH_READ;

    case ePixelShaderTextureRead: return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    case eShaderResourceTextureRead: return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    case eTextureWrite: return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    default: NEVER("invalid resource state %d", static_cast<int>(m_value));
    }
}

DXGI_FORMAT rhi::get_data_format(bundle::DataFormat format) {
    SM_UNUSED constexpr auto refl = ctu::reflect<bundle::DataFormat>();
    using enum bundle::DataFormat::Inner;
    switch (format) {
    case eRGBA8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
    case eRGBA8_UINT: return DXGI_FORMAT_R8G8B8A8_UINT;

    default: NEVER("invalid data format %s", refl.to_string(format).data());
    }
}

#define SM_ASSERT_HR(expr)                                 \
    do {                                                   \
        if (auto result = (expr); FAILED(result)) {        \
            assert_hresult(CT_SOURCE_HERE, #expr, result); \
        }                                                  \
    } while (0)

void DescriptorHeap::init(Device& device, const DescriptorHeapConfig& config) {
    D3D12_DESCRIPTOR_HEAP_FLAGS flags = config.shader_visible
        ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
        : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    D3D12_DESCRIPTOR_HEAP_TYPE type = config.type.as_facade();

    const D3D12_DESCRIPTOR_HEAP_DESC kHeapDesc {
        .Type = type,
        .NumDescriptors = config.size,
        .Flags = flags,
    };

    ID3D12Device1 *api = device.get_device();

    SM_ASSERT_HR(api->CreateDescriptorHeap(&kHeapDesc, IID_PPV_ARGS(&m_heap)));

    m_descriptor_size = api->GetDescriptorHandleIncrementSize(type);
}

void DescriptorArena::init(ID3D12Device1 *device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT length,
                          bool shader_visible) {
    m_slots.resize(length);

    D3D12_DESCRIPTOR_HEAP_FLAGS flags = shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                                                       : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    const D3D12_DESCRIPTOR_HEAP_DESC kHeapDesc {
        .Type = type,
        .NumDescriptors = length,
        .Flags = flags,
    };

    SM_ASSERT_HR(device->CreateDescriptorHeap(&kHeapDesc, IID_PPV_ARGS(&m_heap)));

    m_descriptor_size = device->GetDescriptorHandleIncrementSize(type);
}

void DescriptorArena::release_index(DescriptorIndex index) {
    CTASSERTF(m_slots.test(index), "descriptor heap index %zu is already free", index);
    m_slots.clear(index);
}

DescriptorIndex DescriptorArena::alloc_index() {
    DescriptorIndex index = m_slots.scan_set_first();
    CTASSERTF(index != DescriptorIndex::eInvalid, "descriptor heap exhausted %zu",
              m_slots.get_total_bits());
    return index;
}

void CommandQueue::init(ID3D12Device1 *device, D3D12_COMMAND_LIST_TYPE type, const char *name) {
    const D3D12_COMMAND_QUEUE_DESC kQueueDesc{
        .Type = type,
    };

    SM_ASSERT_HR(device->CreateCommandQueue(&kQueueDesc, IID_PPV_ARGS(&m_queue)));

    SM_ASSERT_HR(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));

    m_event = CreateEventA(nullptr, FALSE, FALSE, name);
    SM_ASSERT_WIN32(m_event != nullptr);
}

void CommandQueue::execute_commands(ID3D12CommandList *const *commands, UINT count) {
    m_queue->ExecuteCommandLists(count, commands);
}

void CommandQueue::signal(UINT64 value) {
    SM_ASSERT_HR(m_queue->Signal(m_fence.get(), value));
}

void CommandQueue::wait(UINT64 value) {
    if (m_fence->GetCompletedValue() < value) {
        SM_ASSERT_HR(m_fence->SetEventOnCompletion(value, m_event));
        SM_ASSERT_WIN32(WaitForSingleObject(m_event, INFINITE) == WAIT_OBJECT_0);
    }
}

void Device::info_callback(D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity,
                            D3D12_MESSAGE_ID id, LPCSTR desc, void *user) {
    MessageCategory cat{category};
    MessageSeverity sev{severity};
    MessageID msg{id};
    Device *device = reinterpret_cast<Device *>(user);

    constexpr auto cat_refl = ctu::reflect<MessageCategory>();
    constexpr auto id_refl = ctu::reflect<MessageID>();

    device->m_log(sev.get_log_severity(), "{} {}: {}", cat_refl.to_string(cat).data(),
               id_refl.to_string(msg).data(), desc);
}

Device::Device(const RenderConfig &config, Factory &factory)
    : m_config(config)
    , m_log(config.logger)
    , m_factory(factory)
    , m_adapter(factory.get_selected_adapter()) {
    init();
}

Device::~Device() {
    await_copy_queue();
    await_direct_queue();
}

void Device::setup_device() {
    SM_UNUSED constexpr auto fl_refl = ctu::reflect<FeatureLevel>();

    auto config = get_config();

    bool debug_device = config.debug_flags.test(DebugFlags::eDeviceDebugLayer);
    bool debug_dred = config.debug_flags.test(DebugFlags::eDeviceRemovedInfo);
    bool debug_queue = config.debug_flags.test(DebugFlags::eInfoQueue);
    bool debug_autoname = config.debug_flags.test(DebugFlags::eAutoName);

    auto fl = config.feature_level;

    if (debug_device) {
        if (HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&m_debug)); SUCCEEDED(hr)) {
            m_debug->EnableDebugLayer();
            m_log.info("enabled d3d12 debug layer");

            Object<ID3D12Debug5> debug5;
            if (debug_autoname && SUCCEEDED(m_debug.query(&debug5))) {
                debug5->SetEnableAutoName(true);
                m_log.info("enabled d3d12 auto naming");
            }
        } else {
            m_log.error("failed to create d3d12 debug interface: {}", hr_string(hr));
        }
    }

    if (debug_dred) {
        Object<ID3D12DeviceRemovedExtendedDataSettings> dred;
        if (HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&dred)); SUCCEEDED(hr)) {
            dred->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            dred->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);

            m_log.info("enabled d3d12 dred");
        } else {
            m_log.error("failed to create d3d12 debug interface: {}", hr_string(hr));
        }
    }

    m_log.info("using adapter: {}", m_adapter.get_adapter_name());

    CTASSERTF(fl.is_valid(), "invalid feature level %s", fl_refl.to_string(fl, 16).data());

    SM_ASSERT_HR(D3D12CreateDevice(m_adapter.get(), fl.as_facade(), IID_PPV_ARGS(&m_device)));

    m_log.info("created d3d12 device");

    if (debug_queue) {
        if (HRESULT hr = m_device->QueryInterface(IID_PPV_ARGS(&m_info_queue)); SUCCEEDED(hr)) {
            m_log.info("created d3d12 info queue");
            SM_ASSERT_HR(m_info_queue->RegisterMessageCallback(
                info_callback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, this, &m_cookie));
        } else {
            m_log.error("failed to create d3d12 info queue: {}", hr_string(hr));
        }
    }

    query_root_signature_version();
}

void Device::setup_render_targets() {
    auto config = get_config();

    for (UINT i = 0; i < config.buffer_count; i++) {
        auto &backbuffer = m_frames[i].backbuffer;
        auto& rtv_index = m_frames[i].rtv_index;

        if (rtv_index == DescriptorIndex::eInvalid)
            rtv_index = m_rtv_heap.alloc_index();

        SM_ASSERT_HR(m_swapchain->GetBuffer(i, IID_PPV_ARGS(&backbuffer)));
        m_device->CreateRenderTargetView(backbuffer.get(), nullptr, m_rtv_heap.get_cpu_handle(rtv_index));
    }
}

void Device::setup_direct_allocators() {
    auto config = get_config();

    for (UINT i = 0; i < config.buffer_count; i++) {
        SM_ASSERT_HR(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                      IID_PPV_ARGS(&m_frames[i].allocator)));
    }
}

void Device::setup_copy_queue() {
    constexpr D3D12_COMMAND_LIST_TYPE kQueueType = D3D12_COMMAND_LIST_TYPE_COPY;

    m_copy_queue.init(m_device.get(), kQueueType, "copy_queue");

    SM_ASSERT_HR(m_device->CreateCommandAllocator(kQueueType, IID_PPV_ARGS(&m_copy_allocator)));

    SM_ASSERT_HR(m_device->CreateCommandList(0, kQueueType, m_copy_allocator.get(), nullptr,
                                             IID_PPV_ARGS(&m_copy_commands)));

    SM_ASSERT_HR(m_copy_commands->Close());
}

void Device::setup_pipeline() {
    auto config = get_config();

    // create command queue
    constexpr D3D12_COMMAND_LIST_TYPE kQueueType = D3D12_COMMAND_LIST_TYPE_DIRECT;
    m_direct_queue.init(m_device.get(), kQueueType, "direct_queue");

    auto client_rect = config.window.get_client_coords();
    HWND hwnd = config.window.get_handle();

    m_swapchain_size = {
        UINT(client_rect.right - client_rect.left),
        UINT(client_rect.bottom - client_rect.top),
    };

    // create swapchain
    const DXGI_SWAP_CHAIN_DESC1 kSwapChainDesc{
        .Width = m_swapchain_size.width,
        .Height = m_swapchain_size.height,
        .Format = get_data_format(config.buffer_format),
        .SampleDesc = {.Count = 1, .Quality = 0},
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = config.buffer_count,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
    };

    auto &factory = m_factory.m_factory;

    Object<IDXGISwapChain1> swapchain;
    SM_ASSERT_HR(factory->CreateSwapChainForHwnd(m_direct_queue.get(), hwnd, &kSwapChainDesc,
                                                 nullptr, nullptr, &swapchain));
    SM_ASSERT_HR(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
    SM_ASSERT_HR(swapchain.query(&m_swapchain));

    m_frame_index = m_swapchain->GetCurrentBackBufferIndex();
    m_frames.reset(config.buffer_count);

    m_log.info("created swapchain");

    // create rtv heap

    m_rtv_heap.init(m_device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, config.rtv_heap_size, false);
    m_srv_heap.init(m_device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                    config.cbv_srv_uav_heap_size, true);

    setup_render_targets();
    setup_direct_allocators();

    // command list
    SM_ASSERT_HR(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                             m_frames[m_frame_index].allocator.get(), nullptr,
                                             IID_PPV_ARGS(&m_commands)));
    SM_ASSERT_HR(m_commands->Close());
}

void Device::init() {
    auto config = get_config();
    constexpr auto df_refl = ctu::reflect<DebugFlags>();
    constexpr auto ap_refl = ctu::reflect<AdapterPreference>();
    constexpr auto fl_refl = ctu::reflect<FeatureLevel>();

    m_log.info("creating rhi context");
    m_log.info(" - flags: {}", df_refl.to_string(config.debug_flags, 2).data());
    m_log.info(" - dsv heap size: {}", config.dsv_heap_size);
    m_log.info(" - rtv heap size: {}", config.rtv_heap_size);
    m_log.info(" - cbv/srv/uav heap size: {}", config.cbv_srv_uav_heap_size);
    m_log.info(" - adapter lookup: {}", ap_refl.to_string(config.adapter_lookup, 10).data());
    m_log.info(" - adapter index: {}", config.adapter_index);
    m_log.info(" - software adapter: {}", config.software_adapter);
    m_log.info(" - feature level: {}", fl_refl.to_string(config.feature_level, 16).data());

    setup_device();
    setup_pipeline();
    setup_copy_queue();
    setup_device_assets();
    setup_camera();
    setup_display_assets();
}

static sm::Vector<uint8_t> load_shader(const char *file) {
    sm::IArena& arena = sm::get_pool(sm::Pool::eGlobal);
    io_t *io = io_file(file, eAccessRead, &arena);
    io_error_t err = io_error(io);
    CTASSERTF(err == 0, "failed to open shader file %s: %s", file, os_error_string(err, &arena));

    size_t size = io_size(io);
    const void *data = io_map(io, eProtectRead);

    CTASSERTF(size != SIZE_MAX, "failed to get size of shader file %s", file);
    CTASSERTF(data != nullptr, "failed to map shader file %s", file);

    sm::Vector<uint8_t> result(size);
    memcpy(result.data(), data, size);

    io_close(io);

    return result;
}

void Device::query_root_signature_version() {
    D3D12_FEATURE_DATA_ROOT_SIGNATURE feature{D3D_ROOT_SIGNATURE_VERSION_1_1};
    if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature,
                                             sizeof(feature)))) {
        m_version = RootSignatureVersion::eVersion_1_0;
    } else {
        m_version = feature.HighestVersion;
    }
}

RootSignatureObject Device::create_root_signature() const {
    RootSignatureObject root_signature;

    CD3DX12_DESCRIPTOR_RANGE1 srv_ranges[1];
    srv_ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

    CD3DX12_DESCRIPTOR_RANGE1 cbv_ranges[1];
    cbv_ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

    CD3DX12_ROOT_PARAMETER1 params[2];
    params[0].InitAsDescriptorTable(std::size(srv_ranges), srv_ranges, D3D12_SHADER_VISIBILITY_PIXEL);
    params[1].InitAsDescriptorTable(std::size(cbv_ranges), cbv_ranges, D3D12_SHADER_VISIBILITY_VERTEX);

    const D3D12_STATIC_SAMPLER_DESC kSampler {
        .Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        .AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        .AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        .AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        .MipLODBias = 0.0f,
        .MaxAnisotropy = 0,
        .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
        .BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
        .MinLOD = 0.0f,
        .MaxLOD = D3D12_FLOAT32_MAX,
        .ShaderRegister = 0,
        .RegisterSpace = 0,
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
    };

    constexpr D3D12_ROOT_SIGNATURE_FLAGS kRootFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
                                                    | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
                                                    | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
                                                    | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rsd;
    rsd.Init_1_1(std::size(params), params, 1, &kSampler, kRootFlags);

    Object<ID3DBlob> signature;
    Object<ID3DBlob> error;
    if (HRESULT hr = D3DX12SerializeVersionedRootSignature(&rsd, m_version.as_facade(), &signature, &error);
        FAILED(hr)) {
        m_log.error("failed to serialize root signature: {}", hr_string(hr));
        std::string_view err{(char *)error->GetBufferPointer(), error->GetBufferSize()};
        m_log.error("{}", err);
        SM_ASSERT_HR(hr);
    }

    SM_ASSERT_HR(m_device->CreateRootSignature(0, signature->GetBufferPointer(),
                                               signature->GetBufferSize(),
                                               IID_PPV_ARGS(&root_signature)));

    return root_signature;
}

PipelineStateObject Device::create_shader_pipeline(RootSignatureObject& root_signature) const {
    auto &config = get_config();

    PipelineStateObject pipeline_state;
    constexpr D3D12_INPUT_ELEMENT_DESC kInputElements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, position),
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, uv),
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    auto vs = load_shader("build/client.exe.p/shader.vs.cso");
    auto ps = load_shader("build/client.exe.p/shader.ps.cso");

    const D3D12_GRAPHICS_PIPELINE_STATE_DESC kPipelineDesc = {
        .pRootSignature = root_signature.get(),
        .VS = {vs.data(), vs.size()},
        .PS = {ps.data(), ps.size()},

        .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        .SampleMask = UINT_MAX,
        .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        .InputLayout = {kInputElements, std::size(kInputElements)},
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = 1,
        .RTVFormats = {get_data_format(config.buffer_format)},
        .SampleDesc = {1, 0},
    };

    SM_ASSERT_HR(
        m_device->CreateGraphicsPipelineState(&kPipelineDesc, IID_PPV_ARGS(&pipeline_state)));

    return pipeline_state;
}

void Device::setup_device_assets() {
    auto signature = create_root_signature();
    auto pipeline = create_shader_pipeline(signature);

    m_pipeline = ShaderPipeline{std::move(signature), std::move(pipeline)};
}

void Device::setup_camera() {
    m_camera.model = math::float4x4::identity();
    m_camera.view = math::float4x4::identity();
    m_camera.projection = math::float4x4::identity();

    constexpr UINT kBufferSize = sizeof(CameraBuffer);
    const CD3DX12_HEAP_PROPERTIES kUploadHeapProperties{D3D12_HEAP_TYPE_UPLOAD};
    const CD3DX12_RESOURCE_DESC kBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(kBufferSize);

    SM_ASSERT_HR(m_device->CreateCommittedResource(
        &kUploadHeapProperties,
        D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
        &kBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_camera_resource)
    ));

    const D3D12_CONSTANT_BUFFER_VIEW_DESC kViewDesc {
        .BufferLocation = m_camera_resource->GetGPUVirtualAddress(),
        .SizeInBytes = kBufferSize,
    };

    m_camera_index = m_srv_heap.alloc_index();
    m_device->CreateConstantBufferView(&kViewDesc, m_srv_heap.get_cpu_handle(m_camera_index));

    CD3DX12_RANGE read_range{0,0};
    SM_ASSERT_HR(m_camera_resource->Map(0, &read_range, reinterpret_cast<void **>(&m_camera_data)));
}

void Device::update_camera() {
    float aspect = get_aspect_ratio();
    m_camera.view = math::float4x4::lookAtRH({1.0f, 1.0f, 1.0f}, {0.1f, 0.1f, 0.1f}, kUpVector).transpose();
    m_camera.projection = math::float4x4::perspectiveRH(60.0f, aspect, 0.1f, 100.0f).transpose();
    *m_camera_data = m_camera;
}

void Device::setup_display_assets() {
    begin_copy();
    open_commands(nullptr);

    m_viewport = {
        .TopLeftX = 0.0f,
        .TopLeftY = 0.0f,
        .Width = float(m_swapchain_size.width),
        .Height = float(m_swapchain_size.height),
        .MinDepth = 0.0f,
        .MaxDepth = 1.0f,
    };

    m_scissor = {
        .left = 0,
        .top = 0,
        .right = LONG(m_swapchain_size.width),
        .bottom = LONG(m_swapchain_size.height),
    };

    const CD3DX12_HEAP_PROPERTIES kDefaultHeapProperties{D3D12_HEAP_TYPE_DEFAULT};
    const CD3DX12_HEAP_PROPERTIES kUploadHeapProperties{D3D12_HEAP_TYPE_UPLOAD};

    // create and upload texture

    constexpr size_t kTextureWidth = 2;
    constexpr size_t kTextureHeight = 2;
    constexpr size_t kPixelSize = sizeof(math::uint8x4);

    constexpr math::uint8x4 kTextureData[kTextureWidth * kTextureHeight] = {
        {255, 0, 0, 255},
        {0, 255, 0, 255},
        {0, 0, 255, 255},
        {255, 255, 255, 255},
    };

    Object<ID3D12Resource> texture_upload;

    const D3D12_RESOURCE_DESC kTextureDesc {
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Width = kTextureWidth,
        .Height = kTextureHeight,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = {1, 0},
        .Flags = D3D12_RESOURCE_FLAG_NONE,
    };

    SM_ASSERT_HR(m_device->CreateCommittedResource(
        &kDefaultHeapProperties,
        D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
        &kTextureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_texture)
    ));

    const UINT64 kUploadSize = GetRequiredIntermediateSize(m_texture.get(), 0, 1);
    const CD3DX12_RESOURCE_DESC kUploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(kUploadSize);

    SM_ASSERT_HR(m_device->CreateCommittedResource(
        &kUploadHeapProperties,
        D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
        &kUploadBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&texture_upload)
    ));

    constexpr size_t kRowPitch = kTextureWidth * kPixelSize;
    constexpr size_t kSlicePitch = kRowPitch * kTextureHeight;

    const D3D12_SUBRESOURCE_DATA kTextureInfo {
        .pData = kTextureData,
        .RowPitch = kRowPitch,
        .SlicePitch = kSlicePitch,
    };

    UpdateSubresources<1>(m_copy_commands.get(), m_texture.get(), texture_upload.get(), 0, 0, 1, &kTextureInfo);

    const CD3DX12_RESOURCE_BARRIER kIntoTexture = CD3DX12_RESOURCE_BARRIER::Transition(
        m_texture.get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    m_commands->ResourceBarrier(1, &kIntoTexture);

    const D3D12_SHADER_RESOURCE_VIEW_DESC kViewDesc {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D = {
            .MipLevels = 1
        },
    };

    if (m_texture_index == DescriptorIndex::eInvalid)
        m_texture_index = m_srv_heap.alloc_index();

    m_log.info("texture index: {}", size_t(m_texture_index));

    m_device->CreateShaderResourceView(m_texture.get(), &kViewDesc, m_srv_heap.get_cpu_handle(m_texture_index));

    // create ibo
    constexpr uint16_t kIndexData[] = {0, 1, 2, 2, 1, 3};
    constexpr UINT kIboSize = sizeof(kIndexData);
    const CD3DX12_RESOURCE_DESC kIndexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(kIboSize);

    m_ibo.try_release();
    Object<ID3D12Resource> ibo_upload;

    SM_ASSERT_HR(m_device->CreateCommittedResource(
        &kDefaultHeapProperties,
        D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
        &kIndexBufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&m_ibo)
    ));

    SM_ASSERT_HR(m_device->CreateCommittedResource(
        &kUploadHeapProperties,
        D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
        &kIndexBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&ibo_upload)
    ));

    UINT8 *ibo_data;
    CD3DX12_RANGE read_range{0,0};
    SM_ASSERT_HR(ibo_upload->Map(0, &read_range, reinterpret_cast<void **>(&ibo_data)));
    memcpy(ibo_data, kIndexData, kIboSize);
    ibo_upload->Unmap(0, nullptr);

    const CD3DX12_RESOURCE_BARRIER kIntoIbo = CD3DX12_RESOURCE_BARRIER::Transition(
        m_ibo.get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_INDEX_BUFFER);

    m_copy_commands->CopyBufferRegion(m_ibo.get(), 0, ibo_upload.get(), 0, kIboSize);
    m_commands->ResourceBarrier(1, &kIntoIbo);

    m_ibo_view = {
        .BufferLocation = m_ibo->GetGPUVirtualAddress(),
        .SizeInBytes = kIboSize,
        .Format = DXGI_FORMAT_R16_UINT,
    };

    // create vbo
    constexpr Vertex kVertexData[] = {
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}},
        {{-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f}},
        {{0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}},
    };

    constexpr UINT kVboSize = sizeof(kVertexData);

    const CD3DX12_RESOURCE_DESC kVertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(kVboSize);

    // upload vbo data
    m_vbo.try_release();
    Object<ID3D12Resource> vbo_upload;

    SM_ASSERT_HR(m_device->CreateCommittedResource(
        &kDefaultHeapProperties,
        D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
        &kVertexBufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&m_vbo)
    ));

    SM_ASSERT_HR(m_device->CreateCommittedResource(
        &kUploadHeapProperties,
        D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
        &kVertexBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&vbo_upload)
    ));

    UINT8 *vbo_data;
    SM_ASSERT_HR(vbo_upload->Map(0, &read_range, reinterpret_cast<void **>(&vbo_data)));
    memcpy(vbo_data, kVertexData, kVboSize);
    vbo_upload->Unmap(0, nullptr);

    const CD3DX12_RESOURCE_BARRIER kIntoBuffer = CD3DX12_RESOURCE_BARRIER::Transition(
        m_vbo.get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    m_copy_commands->CopyBufferRegion(m_vbo.get(), 0, vbo_upload.get(), 0, kVboSize);
    m_commands->ResourceBarrier(1, &kIntoBuffer);

    m_vbo_view = {
        .BufferLocation = m_vbo->GetGPUVirtualAddress(),
        .SizeInBytes = kVboSize,
        .StrideInBytes = sizeof(Vertex),
    };

    submit_copy();
    await_copy_queue();

    SM_ASSERT_HR(m_commands->Close());
    submit_commands();
    await_direct_queue();
}

void Device::open_commands(ID3D12PipelineState *pipeline) {
    auto &[backbuffer, rtv_index, allocator, _] = m_frames[m_frame_index];
    SM_ASSERT_HR(allocator->Reset());

    SM_ASSERT_HR(m_commands->Reset(allocator.get(), pipeline));
}

void Device::record_commands() {
    update_camera();

    open_commands(m_pipeline.get_state());
    auto &[backbuffer, rtv_index, allocator, _] = m_frames[m_frame_index];

    ID3D12DescriptorHeap *heaps[] = {m_srv_heap.get_heap()};
    m_commands->SetDescriptorHeaps(std::size(heaps), heaps);

    m_commands->SetGraphicsRootSignature(m_pipeline.get_signature());

    // use 2 tables in case the indices are not contiguous
    m_commands->SetGraphicsRootDescriptorTable(0, m_srv_heap.get_gpu_handle(m_texture_index));
    m_commands->SetGraphicsRootDescriptorTable(1, m_srv_heap.get_gpu_handle(m_camera_index));
    m_commands->RSSetViewports(1, &m_viewport);
    m_commands->RSSetScissorRects(1, &m_scissor);

    CD3DX12_RESOURCE_BARRIER into_rtv = CD3DX12_RESOURCE_BARRIER::Transition(
        backbuffer.get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    m_commands->ResourceBarrier(1, &into_rtv);

    auto rtv_handle = m_rtv_heap.get_cpu_handle(rtv_index);

    m_commands->OMSetRenderTargets(1, &rtv_handle, FALSE, nullptr);

    constexpr float kClearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};
    m_commands->ClearRenderTargetView(rtv_handle, kClearColor, 0, nullptr);

    m_commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commands->IASetVertexBuffers(0, 1, &m_vbo_view);
    m_commands->IASetIndexBuffer(&m_ibo_view);
    m_commands->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void Device::finish_commands() {
    auto &[backbuffer, index, allocator, _] = m_frames[m_frame_index];
    CD3DX12_RESOURCE_BARRIER into_present = CD3DX12_RESOURCE_BARRIER::Transition(
        backbuffer.get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    m_commands->ResourceBarrier(1, &into_present);
    SM_ASSERT_HR(m_commands->Close());
}

void Device::submit_commands() {
    // later: CommandListCast from d3dx12_core.h
    ID3D12CommandList *commands[] = {m_commands.get()};
    m_direct_queue.execute_commands(commands, std::size(commands));
}

void Device::await_copy_queue() {
    auto value = m_copy_value++;
    m_copy_queue.signal(value);
    m_copy_queue.wait(value);
}

void Device::await_direct_queue() {
    auto value = m_frames[m_frame_index].fence_value++;

    m_direct_queue.signal(value);
    m_direct_queue.wait(value);

    m_frame_index = m_swapchain->GetCurrentBackBufferIndex();
}

float Device::get_aspect_ratio() const {
    auto [width, height] = m_swapchain_size.as<float>();
    return width / height;
}

void Device::resize(math::uint2 size) {
    // dont resize if the size is the same
    if (size == m_swapchain_size) return;

    await_direct_queue();

    auto &config = get_config();

    for (size_t i = 0; i < config.buffer_count; i++) {
        m_frames[i].backbuffer.release();
        m_frames[i].fence_value = m_frames[m_frame_index].fence_value;
    }

    m_log.info("resizing swapchain to {}x{}", size.x, size.y);

    SM_ASSERT_HR(m_swapchain->ResizeBuffers(config.buffer_count, size.x, size.y,
                                            get_data_format(config.buffer_format), 0));

    m_frame_index = m_swapchain->GetCurrentBackBufferIndex();
    m_swapchain_size = size.as<uint32_t>();

    setup_render_targets();
    update_camera();
}

void Device::begin_frame() {
    record_commands();
}

void Device::end_frame() {
    finish_commands();
    submit_commands();
}

void Device::present() {
    SM_ASSERT_HR(m_swapchain->Present(1, 0));
    await_direct_queue();
}

void Device::begin_copy() {
    SM_ASSERT_HR(m_copy_allocator->Reset());
    SM_ASSERT_HR(m_copy_commands->Reset(m_copy_allocator.get(), nullptr));
}

void Device::submit_copy() {
    SM_ASSERT_HR(m_copy_commands->Close());

    ID3D12CommandList *commands[] = {m_copy_commands.get()};
    m_copy_queue.execute_commands(commands, std::size(commands));
}

void Factory::enum_adapters() {
    IDXGIAdapter1 *adapter;
    for (UINT i = 0; m_factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
        Adapter &handle = m_adapters.emplace_back(adapter);

        m_log.info("found adapter: {}", handle.get_adapter_name());
    }
}

void Factory::enum_warp_adapter() {
    IDXGIAdapter1 *adapter;
    SM_ASSERT_HR(m_factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));

    m_adapters.emplace_back(adapter);
}

void Factory::init() {
    auto &config = get_config();
    bool debug_factory = config.debug_flags.test(DebugFlags::eFactoryDebug);

    UINT flags = debug_factory ? DXGI_CREATE_FACTORY_DEBUG : 0u;

    SM_ASSERT_HR(CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_factory)));
    m_log.info("created dxgi factory with flags 0x{:x}", flags);

    if (debug_factory) {
        if (HRESULT hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&m_factory_debug)); SUCCEEDED(hr)) {
            m_log.info("created dxgi debug interface");
            m_factory_debug->EnableLeakTrackingForThread();
        } else {
            m_log.error("failed to create dxgi debug interface: {}", hr_string(hr));
        }
    }

    if (config.software_adapter) {
        enum_warp_adapter();
    } else {
        enum_adapters();
    }
}

Factory::Factory(const RenderConfig &config)
    : m_config(config)
    , m_log(config.logger) {
    init();
}

Factory::~Factory() {
    for (auto &adapter : m_adapters)
        adapter.try_release();

    // report live objects
    if (m_factory_debug) {
        m_log.info("reporting live objects");
        m_factory_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
    }

    m_factory_debug.try_release();
    m_factory.try_release();
}

Adapter &Factory::get_selected_adapter() {
    auto config = get_config();

    auto adapter_index = config.adapter_index;

    CTASSERTF(!m_adapters.empty(), "no adapters found, cannot create context");

    if (m_adapters.size() < adapter_index) {
        m_log.warn("adapter index {} is out of bounds, using first adapter", adapter_index);
        adapter_index = 0;
    }

    return m_adapters[adapter_index];
}

Device Factory::new_device() {
    return Device{get_config(), *this};
}
