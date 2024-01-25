#include "base/panic.h"
#include "io/io.h"
#include "os/os.h"
#include "rhi/rhi.hpp"
#include "core/arena.hpp"

#include <simcoe_config.h>

#include "d3dx12/d3dx12_core.h"
#include "d3dx12/d3dx12_barriers.h"
#include "d3dx12/d3dx12_root_signature.h"

using namespace sm;
using namespace sm::rhi;

extern "C" {
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001; // NOLINT
    __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001; // NOLINT
}

#if SMC_AGILITY_SDK
extern "C" {
    // load the agility sdk
    __declspec(dllexport) extern const UINT D3D12SDKVersion = 611;
    __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
}
#endif

#if SMC_RENDER_DEBUG
#   define SM_RENAME_OBJECT(obj) obj.rename(#obj)
#else
#   define SM_RENAME_OBJECT(obj) ((void)0)
#endif

static char *hr_string(HRESULT hr) {
    sm::IArena *arena = sm::get_debug_arena();
    return os_error_string(hr, arena);
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

#define SM_ASSERT_HR(expr)                                 \
    do {                                                   \
        if (auto result = (expr); FAILED(result)) {        \
            assert_hresult(CT_SOURCE_HERE, #expr, result); \
        }                                                  \
    } while (0)

void Context::info_callback(D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID id, LPCSTR desc, void *user) {
    MessageCategory cat{category};
    MessageSeverity sev{severity};
    MessageID msg{id};
    Context *ctx = reinterpret_cast<Context*>(user);

    constexpr auto cat_refl = ctu::reflect<MessageCategory>();
    constexpr auto id_refl = ctu::reflect<MessageID>();

    ctx->m_log.log(sev.get_log_severity(), "{} {}: {}", cat_refl.to_string(cat).data(), id_refl.to_string(msg).data(), desc);
}

Context::Context(const RenderConfig &config, Factory& factory)
    : m_config(config)
    , m_log(config.logger)
    , m_factory(factory)
    , m_adapter(factory.get_selected_adapter())
{
    init();
}

Context::~Context() {
    wait_for_fence();

    CloseHandle(m_fevent);
}

void Context::setup_device() {
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

            Object<ID3D12Debug5> debug5;
            if (debug_autoname && SUCCEEDED(m_debug.query(&debug5))) {
                debug5->SetEnableAutoName(true);
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

    SM_RENAME_OBJECT(m_device);

    m_log.info("created d3d12 device");

    if (debug_queue) {
        if (HRESULT hr = m_device->QueryInterface(IID_PPV_ARGS(&m_info_queue)); SUCCEEDED(hr)) {
            m_log.info("created d3d12 info queue");
            SM_ASSERT_HR(m_info_queue->RegisterMessageCallback(info_callback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, this, &cookie));
        } else {
            m_log.error("failed to create d3d12 info queue: {}", hr_string(hr));
        }
    }
}

void Context::setup_pipeline() {
    auto config = get_config();

    // create command queue
    const D3D12_COMMAND_QUEUE_DESC kQueueDesc {
        .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
        .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
    };

    SM_ASSERT_HR(m_device->CreateCommandQueue(&kQueueDesc, IID_PPV_ARGS(&m_queue)));
    SM_RENAME_OBJECT(m_queue);

    // fence
    SM_ASSERT_HR(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    SM_RENAME_OBJECT(m_fence);

    // fence handle
    m_fevent = CreateEventA(nullptr, FALSE, FALSE, "rhi.fevent");
    SM_ASSERT_WIN32(m_fevent != nullptr);

    auto window_size = config.window.get_coords();
    HWND hwnd = config.window.get_handle();

    // create swapchain
    const DXGI_SWAP_CHAIN_DESC1 kSwapChainDesc {
        .Width = UINT(window_size.right - window_size.left),
        .Height = UINT(window_size.bottom - window_size.top),
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = {
            .Count = 1,
            .Quality = 0,
        },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = config.buffer_count,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
    };

    auto& factory = m_factory.m_factory;

    Object<IDXGISwapChain1> swapchain;
    SM_ASSERT_HR(factory->CreateSwapChainForHwnd(m_queue.get(), hwnd, &kSwapChainDesc, nullptr, nullptr, &swapchain));
    SM_ASSERT_HR(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
    SM_ASSERT_HR(swapchain.query(&m_swapchain));

    m_frame_index = m_swapchain->GetCurrentBackBufferIndex();
    m_frames.reset(config.buffer_count);

    m_log.info("created swapchain");

    // create rtv heap

    const D3D12_DESCRIPTOR_HEAP_DESC rtv_desc = {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        .NumDescriptors = config.buffer_count,
    };

    SM_ASSERT_HR(m_device->CreateDescriptorHeap(&rtv_desc, IID_PPV_ARGS(&m_rtv_heap)));
    m_rtv_descriptor_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(m_rtv_heap->GetCPUDescriptorHandleForHeapStart());

    for (UINT i = 0; i < config.buffer_count; i++) {
        auto& backbuffer = m_frames[i].backbuffer;
        SM_ASSERT_HR(m_swapchain->GetBuffer(i, IID_PPV_ARGS(&backbuffer)));
        m_device->CreateRenderTargetView(backbuffer.get(), nullptr, rtv_handle);
        rtv_handle.Offset(1, m_rtv_descriptor_size);

        SM_ASSERT_HR(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_frames[i].allocator)));
    }

    // command list
    SM_ASSERT_HR(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_frames[m_frame_index].allocator.get(), nullptr, IID_PPV_ARGS(&m_commands)));
    SM_ASSERT_HR(m_commands->Close());
}

void Context::init() {
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
    setup_assets();
}

static std::vector<uint8_t> load_shader(const char *file) {
    sm::IArena *arena = sm::get_debug_arena();
    io_t *io = io_file(file, eAccessRead, arena);
    io_error_t err = io_error(io);
    CTASSERTF(err == 0, "failed to open shader file %s: %s", file, os_error_string(err, arena));

    size_t size = io_size(io);
    const void *data = io_map(io); // TODO: leaks!!!

    CTASSERTF(size != SIZE_MAX, "failed to get size of shader file %s", file);
    CTASSERTF(data != nullptr, "failed to map shader file %s", file);

    std::vector<uint8_t> result(size);
    memcpy(result.data(), data, size);

    io_close(io);

    return result;
}

void Context::setup_assets() {
    SM_UNUSED constexpr auto v_refl = ctu::reflect<RootSignatureVersion>();
    RootSignatureVersion version = RootSignatureVersion::eVersion_1_0;
    CTASSERTF(version.is_valid(), "invalid pipeline state version %s", v_refl.to_string(version, 2).data());

    Object<ID3D12RootSignature> root_signature;
    Object<ID3D12PipelineState> pipeline_state;

    CD3DX12_ROOT_SIGNATURE_DESC rsd;
    rsd.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    Object<ID3DBlob> signature;
    Object<ID3DBlob> error;
    if (HRESULT hr = D3D12SerializeRootSignature(&rsd, version.as_facade(), &signature, &error); FAILED(hr)) {
        m_log.error("failed to serialize root signature: {}", hr_string(hr));
        std::string_view err { (char*)error->GetBufferPointer(), error->GetBufferSize() };
        m_log.error("{}", err);
        SM_ASSERT_HR(hr);
    }

    SM_ASSERT_HR(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&root_signature)));

    D3D12_INPUT_ELEMENT_DESC input_elements[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOUR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, colour), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    auto vs = load_shader("build/client.exe.p/shader.vs.cso");
    auto ps = load_shader("build/client.exe.p/shader.ps.cso");

    SM_UNUSED D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {
        .pRootSignature = root_signature.get(),
        .VS = { vs.data(), vs.size() },
        .PS = { ps.data(), ps.size() },

        .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        .SampleMask = UINT_MAX,
        .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        .InputLayout = { input_elements, 2 },
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = 1,
        .RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM },
        .SampleDesc = { 1, 0 },
    };

    SM_ASSERT_HR(m_device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&pipeline_state)));

    m_state = std::move(pipeline_state);
    m_signature = std::move(root_signature);

    sys::WindowCoords coords = get_window().get_coords();

    auto width = coords.right - coords.left;
    auto height = coords.bottom - coords.top;

    m_viewport = {
        .TopLeftX = 0.0f,
        .TopLeftY = 0.0f,
        .Width = float(width),
        .Height = float(height),
        .MinDepth = 0.0f,
        .MaxDepth = 1.0f,
    };

    m_scissor = {
        .left = 0,
        .top = 0,
        .right = LONG(width),
        .bottom = LONG(height),
    };

    // create vbo
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    const Vertex vertices[] = {
        { { 0.0f, 0.25f * aspect, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { 0.25f, -0.25f * aspect, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { -0.25f, -0.25f * aspect, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
    };

    const UINT kVboSize = sizeof(vertices);

    const CD3DX12_HEAP_PROPERTIES kUploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
    const CD3DX12_RESOURCE_DESC kUploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(kVboSize);

    SM_ASSERT_HR(m_device->CreateCommittedResource(
        &kUploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &kUploadBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_vbo)));

    UINT8 *vbo_data;
    CD3DX12_RANGE read_range(0, 0);
    SM_ASSERT_HR(m_vbo->Map(0, &read_range, reinterpret_cast<void**>(&vbo_data)));
    memcpy(vbo_data, vertices, kVboSize);
    m_vbo->Unmap(0, nullptr);

    m_vbo_view = {
        .BufferLocation = m_vbo->GetGPUVirtualAddress(),
        .SizeInBytes = kVboSize,
        .StrideInBytes = sizeof(Vertex),
    };
}

void Context::setup_commands() {
    auto& [backbuffer, allocator, _] = m_frames[m_frame_index];
    SM_ASSERT_HR(allocator->Reset());

    SM_ASSERT_HR(m_commands->Reset(allocator.get(), m_state.get()));

    m_commands->SetGraphicsRootSignature(m_signature.get());
    m_commands->RSSetViewports(1, &m_viewport);
    m_commands->RSSetScissorRects(1, &m_scissor);

    CD3DX12_RESOURCE_BARRIER into_rtv = CD3DX12_RESOURCE_BARRIER::Transition(backbuffer.get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    CD3DX12_RESOURCE_BARRIER into_present = CD3DX12_RESOURCE_BARRIER::Transition(backbuffer.get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    m_commands->ResourceBarrier(1, &into_rtv);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(m_rtv_heap->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(m_frame_index), m_rtv_descriptor_size);

    m_commands->OMSetRenderTargets(1, &rtv_handle, FALSE, nullptr);

    constexpr float kClearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_commands->ClearRenderTargetView(rtv_handle, kClearColor, 0, nullptr);

    m_commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commands->IASetVertexBuffers(0, 1, &m_vbo_view);
    m_commands->DrawInstanced(3, 1, 0, 0);

    m_commands->ResourceBarrier(1, &into_present);

    SM_ASSERT_HR(m_commands->Close());
}

void Context::submit_commands() {
    // later: CommandListCast from d3dx12_core.h
    ID3D12CommandList *commands[] = { m_commands.get() };
    m_queue->ExecuteCommandLists(1, commands);
}

void Context::wait_for_fence() {
    auto& value = m_frames[m_frame_index].fence_value;
    SM_ASSERT_HR(m_queue->Signal(m_fence.get(), value));

    if (m_fence->GetCompletedValue() < value) {
        SM_ASSERT_HR(m_fence->SetEventOnCompletion(value, m_fevent));
        SM_ASSERT_WIN32(WaitForSingleObject(m_fevent, INFINITE) == WAIT_OBJECT_0);
    }

    value++;
    m_frame_index = m_swapchain->GetCurrentBackBufferIndex();
}

float Context::get_aspect_ratio() const {
    auto size = get_window().get_coords();
    return float(size.right - size.left) / float(size.bottom - size.top);
}

void Context::begin_frame() {
    setup_commands();
}

void Context::end_frame() {
    submit_commands();

    SM_ASSERT_HR(m_swapchain->Present(1, 0));

    wait_for_fence();
}

void Factory::enum_adapters() {
    IDXGIAdapter1 *adapter;
    for (UINT i = 0; m_factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
        Adapter& handle = m_adapters.emplace_back(adapter);

        m_log.info("found adapter: {}", handle.get_adapter_name());
    }
}

void Factory::enum_warp_adapter() {
    IDXGIAdapter1 *adapter;
    SM_ASSERT_HR(m_factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));

    m_adapters.emplace_back(adapter);
}

void Factory::init() {
    auto& config = get_config();
    bool debug_factory = config.debug_flags.test(DebugFlags::eFactoryDebug);

    UINT flags = debug_factory ? DXGI_CREATE_FACTORY_DEBUG : 0u;

    SM_ASSERT_HR(CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_factory)));
    m_log.info("created dxgi factory");

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

Factory::Factory(const RenderConfig& config)
    : m_config(config)
    , m_log(config.logger)
{
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

Adapter& Factory::get_selected_adapter() {
    auto config = get_config();

    auto adapter_index = config.adapter_index;

    CTASSERTF(!m_adapters.empty(), "no adapters found, cannot create context");

    if (m_adapters.size() < adapter_index) {
        m_log.warn("adapter index {} is out of bounds, using first adapter", adapter_index);
        adapter_index = 0;
    }

    return m_adapters[adapter_index];
}

Context Factory::new_context() {
    return Context{get_config(), *this};
}
