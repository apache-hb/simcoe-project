#include "rhi/rhi.hpp"
#include "base/panic.h"
#include "core/arena.hpp"
#include "os/core.h"

#include "d3dx12/d3dx12_barriers.h"

#include "core/reflect.hpp" // IWYU pragma: export
#include "math/format.hpp"  // IWYU pragma: export
#include "i18n/i18n.hpp"

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

char *sm::rhi::hresult_string(HRESULT hr) {
    sm::IArena& arena = sm::get_pool(sm::Pool::eDebug);
    return os_error_string(hr, &arena);
}

CT_NORETURN
sm::rhi::assert_hresult(source_info_t source, const char *expr, HRESULT hr) {
    ctu_panic(source, "hresult: %s %s", hresult_string(hr), expr);
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
    using Reflect = ctu::TypeInfo<ResourceState>;
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

    case eTextureRead: return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    case eShaderTextureRead: return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    case eTextureWrite: return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    default: NEVER("invalid resource state %s", Reflect::to_string(m_value).data());
    }
}

D3D12_DESCRIPTOR_RANGE_TYPE BindingType::get_range_type() const {
    using Reflect = ctu::TypeInfo<BindingType>;
    using enum BindingType::Inner;
    switch (m_value) {
    case eTexture: return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    case eConstBuffer: return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;

    default: NEVER("invalid binding type %s", Reflect::to_string(m_value).data());
    }
}

DXGI_FORMAT rhi::get_data_format(bundle::DataFormat format) {
    using Reflect = ctu::TypeInfo<bundle::DataFormat>;
    using enum bundle::DataFormat::Inner;
    switch (format) {
    case eRGBA8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
    case eRGBA8_UINT: return DXGI_FORMAT_R8G8B8A8_UINT;
    case eRG32_FLOAT: return DXGI_FORMAT_R32G32_FLOAT;
    case eRGB32_FLOAT: return DXGI_FORMAT_R32G32B32_FLOAT;
    case eRGBA32_FLOAT: return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case eUINT16: return DXGI_FORMAT_R16_UINT;
    case eUINT32: return DXGI_FORMAT_R32_UINT;

    default: NEVER("invalid data format %s", Reflect::to_string(format).data());
    }
}

void DescriptorHeap::init(Device& device, const DescriptorHeapConfig& config) {
    D3D12_DESCRIPTOR_HEAP_FLAGS flags = config.shader_visible
        ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
        : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    D3D12_DESCRIPTOR_HEAP_TYPE type = config.type.as_facade();

    const D3D12_DESCRIPTOR_HEAP_DESC kHeapDesc = {
        .Type = type,
        .NumDescriptors = config.size,
        .Flags = flags,
    };

    ID3D12Device1 *api = device.get_device();

    SM_ASSERT_HR(api->CreateDescriptorHeap(&kHeapDesc, IID_PPV_ARGS(&m_heap)));

    m_descriptor_size = api->GetDescriptorHandleIncrementSize(type);
}

void DescriptorArena::init(Device& device, const DescriptorHeapConfig& config) {
    m_slots.resize(config.size);
    m_heap.init(device, config);
}

void DescriptorArena::release_index(DescriptorIndex index) {
    CTASSERTF(m_slots.test(index), "descriptor heap index %zu is already free", index);
    m_slots.clear(index);
}

DescriptorIndex DescriptorArena::alloc_index() {
    DescriptorIndex index = m_slots.scan_set_first();
    CTASSERTF(index != DescriptorIndex::eInvalid, "descriptor heap exhausted %zu",
              m_slots.get_capacity());
    return index;
}

void Device::info_callback(D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity,
                            D3D12_MESSAGE_ID id, LPCSTR desc, void *user) {
    MessageCategory cat{category};
    MessageSeverity sev{severity};
    MessageID msg{id};
    Device *device = reinterpret_cast<Device *>(user);

    device->m_log(sev.get_log_severity(), "{} {}: {}", cat, msg, desc);
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
    SM_UNUSED constexpr auto refl = ctu::reflect<FeatureLevel>();

    auto config = get_config();

    bool debug_device = config.debug_flags.test(DebugFlags::eDeviceDebugLayer);
    bool debug_dred = config.debug_flags.test(DebugFlags::eDeviceRemovedInfo);
    bool debug_queue = config.debug_flags.test(DebugFlags::eInfoQueue);
    bool debug_autoname = config.debug_flags.test(DebugFlags::eAutoName);

    auto fl = config.feature_level;

    if (debug_device) {
        if (HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&m_debug)); SUCCEEDED(hr)) {
            m_debug->EnableDebugLayer();

            Object<ID3D12Debug5> debug5;
            if (debug_autoname && SUCCEEDED(m_debug.query(&debug5))) {
                debug5->SetEnableAutoName(true);
            }
        } else {
            m_log.error("failed to create d3d12 debug interface: {}", hresult_string(hr));
        }
    }

    if (debug_dred) {
        Object<ID3D12DeviceRemovedExtendedDataSettings> dred;
        if (HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&dred)); SUCCEEDED(hr)) {
            dred->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            dred->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        } else {
            m_log.error("failed to create d3d12 debug interface: {}", hresult_string(hr));
        }
    }

    m_log.info("using adapter: {}", m_adapter.get_adapter_name());

    CTASSERTF(fl.is_valid(), "invalid feature level %s", refl.to_string(fl, 16).data());

    SM_ASSERT_HR(D3D12CreateDevice(m_adapter.get(), fl.as_facade(), IID_PPV_ARGS(&m_device)));

    if (debug_queue) {
        if (HRESULT hr = m_device->QueryInterface(IID_PPV_ARGS(&m_info_queue)); SUCCEEDED(hr)) {
            SM_ASSERT_HR(m_info_queue->RegisterMessageCallback(
                info_callback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, this, &m_cookie));
        } else {
            m_log.error("failed to create d3d12 info queue: {}", hresult_string(hr));
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
        m_frames[i].allocator.init(*this, CommandListType::eDirect);
    }
}

void Device::setup_copy_queue() {
    constexpr CommandListType kQueueType = CommandListType::eDirect;

    m_copy_queue.init(*this, kQueueType, "copy_queue");

    m_copy_allocator.init(*this, kQueueType);

    m_copy_list.init(*this, m_copy_allocator, kQueueType);
    m_copy_list.close();
}

void Device::setup_direct_queue() {
    auto config = get_config();

    // create command queue
    constexpr D3D12_COMMAND_LIST_TYPE kQueueType = D3D12_COMMAND_LIST_TYPE_DIRECT;
    m_direct_queue.init(*this, kQueueType, "direct_queue");

    setup_swapchain();

    // create rtv heap

    const DescriptorHeapConfig kHeapConfig = {
        .type = DescriptorHeapType::eRTV,
        .size = config.rtv_heap_size,
        .shader_visible = false,
    };

    m_rtv_heap.init(*this, kHeapConfig);

    setup_render_targets();
    setup_direct_allocators();

    m_direct_list.init(*this, m_frames[m_frame_index].allocator, kQueueType);
    m_direct_list.close();
}

void Device::setup_swapchain() {
    auto config = get_config();

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
}

void Device::init() {
    setup_device();
    query_features();
    setup_direct_queue();
    setup_copy_queue();
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

void Device::query_features() {
    D3D12_FEATURE_DATA_D3D12_OPTIONS16 options{};
    if (SUCCEEDED(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS16, &options,
                                                sizeof(options)))) {
        m_features.gpu_upload_heap = options.GPUUploadHeapSupported;
    }

    m_log.info("| gpu upload heap: {}", (bool)m_features.gpu_upload_heap);
}

void Device::record_commands() {
    auto &[backbuffer, rtv_index, allocator, _] = m_frames[m_frame_index];

    CD3DX12_RESOURCE_BARRIER into_rtv = CD3DX12_RESOURCE_BARRIER::Transition(
        backbuffer.get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    m_direct_list->ResourceBarrier(1, &into_rtv);

    auto rtv_handle = m_rtv_heap.get_cpu_handle(rtv_index);

    m_direct_list->OMSetRenderTargets(1, &rtv_handle, FALSE, nullptr);

    constexpr float kClearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};
    m_direct_list->ClearRenderTargetView(rtv_handle, kClearColor, 0, nullptr);
}

void Device::finish_commands() {
    auto &[backbuffer, index, allocator, _] = m_frames[m_frame_index];
    CD3DX12_RESOURCE_BARRIER into_present = CD3DX12_RESOURCE_BARRIER::Transition(
        backbuffer.get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    m_direct_list->ResourceBarrier(1, &into_present);
    m_direct_list.close();
}

void Device::submit_commands() {
    // later: CommandListCast from d3dx12_core.h
    ID3D12CommandList *commands[] = {m_direct_list.get()};
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

    m_log.info("resizing swapchain to {}", size);

    SM_ASSERT_HR(m_swapchain->ResizeBuffers(config.buffer_count, size.x, size.y,
                                            get_data_format(config.buffer_format), 0));

    m_frame_index = m_swapchain->GetCurrentBackBufferIndex();
    m_swapchain_size = size.as<uint32_t>();

    setup_render_targets();
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

void Device::flush_copy_commands(CommandListObject& commands) {
    SM_ASSERT_HR(commands->Close());

    ID3D12CommandList *list[] = {commands.get()};
    m_copy_queue.execute_commands(list, std::size(list));

    await_copy_queue();
}

void Device::flush_direct_commands(CommandListObject& commands) {
    SM_ASSERT_HR(commands->Close());

    ID3D12CommandList *list[] = {commands.get()};
    m_direct_queue.execute_commands(list, std::size(list));

    await_direct_queue();
}

void Factory::enum_adapters() {
    IDXGIAdapter1 *adapter;
    for (UINT i = 0; m_factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
        Adapter &handle = m_adapters.emplace_back(adapter);

        m_log.info("| {}: {}", i, handle.get_adapter_name());
    }
}

void Factory::enum_warp_adapter() {
    IDXGIAdapter1 *adapter;
    SM_ASSERT_HR(m_factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));

    Adapter& handle = m_adapters.emplace_back(adapter);

    m_log.info("| warp: {}", handle.get_adapter_name());
}

void Factory::init() {
    auto &config = get_config();

    m_log.info("creating rhi context");
    m_log.info("| flags: {:b}", config.debug_flags);
    m_log.info("| rtv heap size: {}", config.rtv_heap_size);
    m_log.info("| adapter lookup: {}", config.adapter_lookup);
    m_log.info("| adapter index: {}", config.adapter_index);
    m_log.info("| software adapter: {}", config.software_adapter);
    m_log.info("| feature level: {:x}", config.feature_level);

    bool debug_factory = config.debug_flags.test(DebugFlags::eFactoryDebug);

    UINT flags = debug_factory ? DXGI_CREATE_FACTORY_DEBUG : 0u;

    SM_ASSERT_HR(CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_factory)));

    if (debug_factory) {
        if (HRESULT hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&m_factory_debug)); SUCCEEDED(hr)) {
            m_factory_debug->EnableLeakTrackingForThread();
        } else {
            m_log.error("failed to create dxgi debug interface: {}", hresult_string(hr));
        }
    }

    bool warp = config.software_adapter;

    m_log.info("enumerating {} {}", warp ? "warp" : "all", i18n::plural("adapter", !warp));

    if (warp) {
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
