#include "os/os.h"
#include "rhi/rhi.hpp"
#include "core/arena.hpp"

using namespace sm;
using namespace sm::rhi;

NORETURN
static assert_hresult(source_info_t panic, const char *expr, HRESULT hr) {
    sm::IArena *arena = sm::get_debug_arena();
    char *message = os_error_string(hr, arena);

    ctu_panic(panic, "hresult: %s %s", message, expr);
}

#define SM_ASSERT_HR(expr)                                 \
    do {                                                   \
        if (auto result = (expr); FAILED(result)) {        \
            assert_hresult(CT_SOURCE_HERE, #expr, result); \
        }                                                  \
    } while (0)

void Context::enum_adapters() {
    IDXGIAdapter1 *adapter;
    for (UINT i = 0; m_factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
        Adapter& handle = m_adapters.emplace_back(adapter);

        m_log.info("found adapter: {}", handle.get_adapter_name());
    }
}

void Context::enum_warp_adapter() {
    IDXGIAdapter1 *adapter;
    SM_ASSERT_HR(m_factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));

    m_adapters.emplace_back(adapter);
}

Context::Context(const RenderConfig &config)
    : m_config(config)
    , m_log(config.logger)
{
    init();
}

Context::~Context() {
    m_device.try_release();
    for (auto &adapter : m_adapters)
        adapter.try_release();

    // report live objects
    if (m_factory_debug) {
        m_factory_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
    }

    m_factory.try_release();
}

void Context::init() {
    auto config = get_config();
    constexpr auto fl_refl = ctu::reflect<FeatureLevel>();
    constexpr auto df_refl = ctu::reflect<DebugFlags>();
    constexpr auto ap_refl = ctu::reflect<AdapterPreference>();

    m_log.info("creating rhi context");

    m_log.info(" - flags: {}", df_refl.to_string(config.debug_flags, 2).data());
    m_log.info(" - dsv heap size: {}", config.dsv_heap_size);
    m_log.info(" - rtv heap size: {}", config.rtv_heap_size);
    m_log.info(" - cbv/srv/uav heap size: {}", config.cbv_srv_uav_heap_size);
    m_log.info(" - adapter lookup: {}", ap_refl.to_string(config.adapter_lookup, 10).data());
    m_log.info(" - adapter index: {}", config.adapter_index);
    m_log.info(" - software adapter: {}", config.software_adapter);
    m_log.info(" - feature level: {}", fl_refl.to_string(config.feature_level, 16).data());

    bool debug_factory = config.debug_flags.test(DebugFlags::eFactoryDebug);

    UINT flags = debug_factory ? DXGI_CREATE_FACTORY_DEBUG : 0u;

    SM_ASSERT_HR(CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_factory)));
    m_log.info("created dxgi factory");

    if (debug_factory) {
        if (HRESULT hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&m_factory_debug)); FAILED(hr)) {
            m_log.error("failed to create dxgi debug interface: {}", os_error_string(hr, get_debug_arena()));
        } else {
            m_log.info("created dxgi debug interface");
        }
    }

    if (config.software_adapter) {
        m_log.info("overriding adapter enumeration with software adapter");
        enum_warp_adapter();
    } else {
        enum_adapters();
    }

    CTASSERTF(!m_adapters.empty(), "no adapters found, perhaps you need to enable software rendering?");

    auto adapter_index = config.adapter_index;

    if (m_adapters.size() < adapter_index) {
        m_log.warn("adapter index {} is out of bounds, using first adapter", adapter_index);
        adapter_index = 0;
    }

    auto &adapter = m_adapters[adapter_index];

    m_log.info("using adapter: {}", adapter.get_adapter_name());

    CTASSERTF(config.feature_level.is_valid(), "invalid feature level %s", fl_refl.to_string(config.feature_level, 16).data());

    SM_ASSERT_HR(D3D12CreateDevice(adapter.get(), config.feature_level.as_facade(), IID_PPV_ARGS(&m_device)));

}
