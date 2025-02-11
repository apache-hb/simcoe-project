#include "stdafx.hpp"

#include "render/base/instance.hpp"

#include "db/connection.hpp"
#include "db/transaction.hpp"

#include "render.dao.hpp"

using namespace sm;
using namespace sm::render;

Adapter::Adapter(IDXGIAdapter1 *adapter)
    : Object(adapter)
{
    DXGI_ADAPTER_DESC1 desc;
    SM_THROW_HR(adapter->GetDesc1(&desc));

    mName = sm::narrow(desc.Description);
    mVideoMemory = desc.DedicatedVideoMemory;
    mSystemMemory = desc.DedicatedSystemMemory;
    mSharedMemory = desc.SharedSystemMemory;
    mFlags = desc.Flags;
    mAdapterLuid = desc.AdapterLuid;

    mDeviceInfo = {
        .vendor = desc.VendorId,
        .device = desc.DeviceId,
        .subsystem = desc.SubSysId,
        .revision = desc.Revision,
    };
}

bool Instance::enumAdaptersByPreference() {
    Object<IDXGIFactory6> factory6;
    if (Result hr = mFactory.query(&factory6); !hr) {
        LOG_WARN(GpuLog, "Failed to query interface IDXGIFactory6: {}.", hr);
        return false;
    }

    auto enumAdapter = [&](UINT i, IDXGIAdapter1 **adapter) {
        return factory6->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE(mAdapterSearch), IID_PPV_ARGS(adapter)) != DXGI_ERROR_NOT_FOUND;
    };

    IDXGIAdapter1 *adapter;
    for (UINT i = 0; enumAdapter(i, &adapter); i++) {
        mAdapters.emplace_back(adapter);
    }

    return true;
}

void Instance::enumAdapters() {
    IDXGIAdapter1 *adapter;
    for (UINT i = 0; mFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        mAdapters.emplace_back(adapter);
    }
}

void Instance::findWarpAdapter() {
    IDXGIAdapter1 *adapter;
    if (Result hr = mFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)); !hr) {
        LOG_WARN(GpuLog, "Failed to enumerate WARP adapter: {}.", hr);
        return;
    }

    mWarpAdapter = Adapter(adapter);
}

void Instance::enableDebugLeakTracking() {
    if (Result hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&mDebug))) {
        mDebug->EnableLeakTrackingForThread();
        LOG_INFO(GpuLog, "Enabled DXGI debug interface.");
    } else {
        LOG_ERROR(GpuLog, "Failed to enable DXGI debug interface: {}.", hr);
    }
}

bool Instance::queryTearingSupport() const {
    Object<IDXGIFactory5> factory;
    if (Result hr = mFactory.query(&factory); !hr) {
        LOG_WARN(GpuLog, "Failed to query interface IDXGIFactory5: {}.", hr);
        return false;
    }

    BOOL tearing = FALSE;
    if (Result hr = factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearing, sizeof(tearing)); !hr) {
        LOG_WARN(GpuLog, "CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING) failed: {}.", hr);
        return false;
    }

    return tearing;
}

void Instance::loadWarpRedist() {
#if SMC_WARP_ENABLE
    mWarpLibrary = sm::get_redist("d3d10warp.dll");
    if (OsError err = mWarpLibrary.get_error()) {
        LOG_WARN(GpuLog, "failed to load warp redist: {}", err);
    } else {
        LOG_INFO(GpuLog, "loaded warp redist");
    }
#endif
}

void Instance::loadPIXRuntime() {
#if SMC_PIX_ENABLE
    if (PIXLoadLatestWinPixGpuCapturerLibrary()) {
        LOG_INFO(GpuLog, "loaded pix runtime");
    } else {
        LOG_WARN(GpuLog, "failed to load pix runtime: {}", sys::getLastError());
    }
#endif
}

Instance::Instance(InstanceConfig config) noexcept(false)
    : mFlags(config.flags)
    , mAdapterSearch(config.preference)
{
    bool debug = bool(mFlags & DebugFlags::eFactoryDebug);
    const UINT flags = debug ? DXGI_CREATE_FACTORY_DEBUG : 0;
    SM_THROW_HR(CreateDXGIFactory2(flags, IID_PPV_ARGS(&mFactory)));

    mTearingSupport = queryTearingSupport();
    LOG_INFO(GpuLog, "Instance has tearing support: {}.", mTearingSupport ? "yes" : "no");

    if (debug)
        enableDebugLeakTracking();

    loadWarpRedist();

    if (bool(mFlags & DebugFlags::eWinPixEventRuntime))
        loadPIXRuntime();

    findWarpAdapter();

    if (!enumAdaptersByPreference())
        enumAdapters();
}

Instance::~Instance() noexcept {
    if (!mDebug) return;

    LOG_INFO(GpuLog, "Reporting live DXGI/D3D objects.");
    reportLiveObjects();
}

bool Instance::hasViableAdapter() const noexcept {
    return mWarpAdapter.isValid() || !mAdapters.empty();
}

Adapter *Instance::getAdapterByLUID(LUID luid) noexcept {
    for (Adapter &adapter : mAdapters) {
        if (adapter.luid() == luid) {
            return std::addressof(adapter);
        }
    }

    return nullptr;
}

Adapter *Instance::findAdapterByLUID(AdapterLUID luid) noexcept {
    for (Adapter &adapter : mAdapters) {
        if (adapter == luid) {
            return std::addressof(adapter);
        }
    }

    return nullptr;
}

Adapter& Instance::getWarpAdapter() noexcept {
    return mWarpAdapter;
}

Adapter& Instance::getDefaultAdapter() noexcept {
    return mAdapters.empty() ? getWarpAdapter() : mAdapters.front();
}

bool Instance::isDebugEnabled() const noexcept {
    return bool(mFlags & DebugFlags::eFactoryDebug);
}

bool Instance::reportLiveObjects(DXGI_DEBUG_RLO_FLAGS flags) noexcept {
    if (mDebug) {
        mDebug->ReportLiveObjects(DXGI_DEBUG_ALL, flags);
        return true;
    }

    return false;
}

namespace renderdao = sm::dao::render;

static uint64_t packLuid(LUID luid) {
    return uint64_t(luid.HighPart) << 32 | luid.LowPart;
}

class SaveAdapterInfo {
    sm::db::Connection& mConnection;

    sm::db::PreparedInsertReturning<renderdao::Mode> mInsertMode = mConnection.prepareInsertReturningPrimaryKey<renderdao::Mode>();
    sm::db::PreparedInsertReturning<renderdao::Output> mInsertOutput = mConnection.prepareInsertReturningPrimaryKey<renderdao::Output>();
    sm::db::PreparedInsert<renderdao::Adapter> mInsertAdapter = mConnection.prepareInsertOrUpdate<renderdao::Adapter>();

    void addMode(DXGI_MODE_DESC desc, uint64_t output) {
        renderdao::Mode dao {
            .output = output,
            .width = desc.Width,
            .height = desc.Height,
            .refreshRateNumerator = desc.RefreshRate.Numerator,
            .refreshRateDenominator = desc.RefreshRate.Denominator,
            .format = uint32_t(desc.Format),
            .scanlineOrdering = uint32_t(desc.ScanlineOrdering),
            .scaling = uint32_t(desc.Scaling),
        };

        mInsertMode.insert(dao);
    }

    void addOutput(IDXGIOutput *output, uint64_t luid, DXGI_FORMAT format) {
        DXGI_OUTPUT_DESC desc;
        SM_THROW_HR(output->GetDesc(&desc));

        renderdao::Output dao {
            .name = sm::narrow(desc.DeviceName),
            .left = desc.DesktopCoordinates.left,
            .top = desc.DesktopCoordinates.top,
            .right = desc.DesktopCoordinates.right,
            .bottom = desc.DesktopCoordinates.bottom,
            .attachedToDesktop = !!desc.AttachedToDesktop,
            .rotation = uint32_t(desc.Rotation),
            .adapter = luid,
        };

        uint64_t pk = mInsertOutput.insert(dao);

        const UINT flags = DXGI_ENUM_MODES_INTERLACED;

        UINT count = 0;
        output->GetDisplayModeList(format, flags, &count, nullptr);

        std::unique_ptr<DXGI_MODE_DESC[]> modes(new DXGI_MODE_DESC[count]);
        SM_THROW_HR(output->GetDisplayModeList(format, flags, &count, modes.get()));

        for (UINT i = 0; i < count; i++) {
            addMode(modes[i], pk);
        }
    }

    void addAdapter(const Adapter& adapter, DXGI_FORMAT format) {
        DXGI_ADAPTER_DESC1 desc;
        SM_THROW_HR(adapter->GetDesc1(&desc));

        uint64_t luid = packLuid(desc.AdapterLuid);

        renderdao::Adapter dao {
            .luid = luid,
            .description = std::string{adapter.name()},
            .flags = desc.Flags,
            .videoMemory = adapter.vidmem().asBytes(),
            .systemMemory = adapter.sysmem().asBytes(),
            .sharedMemory = adapter.sharedmem().asBytes(),
            .vendorId = desc.VendorId,
            .deviceId = desc.DeviceId,
            .subsystemId = desc.SubSysId,
            .revision = desc.Revision,
        };

        mInsertAdapter.insert(dao);

        IDXGIOutput *output = nullptr;
        for (UINT i = 0; adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND; ++i) {
            addOutput(output, luid, format);
        }
    }

    void addRotations() {
        auto stmt = mConnection.prepareInsertOrUpdate<renderdao::Rotation>();

        // add rotations
        renderdao::Rotation rotations[] = {
            { DXGI_MODE_ROTATION_UNSPECIFIED, "DXGI_MODE_ROTATION_UNSPECIFIED" },
            { DXGI_MODE_ROTATION_IDENTITY,    "DXGI_MODE_ROTATION_IDENTITY" },
            { DXGI_MODE_ROTATION_ROTATE90,    "DXGI_MODE_ROTATION_ROTATE90" },
            { DXGI_MODE_ROTATION_ROTATE180,   "DXGI_MODE_ROTATION_ROTATE180" },
            { DXGI_MODE_ROTATION_ROTATE270,   "DXGI_MODE_ROTATION_ROTATE270" },
        };

        for (const auto& rotation : rotations) {
            stmt.insert(rotation);
        }
    }

public:
    SaveAdapterInfo(sm::db::Connection& connection)
        : mConnection(connection)
    {
        mConnection.truncate<renderdao::Mode>();
        mConnection.truncate<renderdao::Output>();
        mConnection.truncate<renderdao::Adapter>();
    }

    void submitInfo(const Instance& instance, DXGI_FORMAT format) {
        db::Transaction tx(&mConnection);
        addRotations();

        for (const Adapter& adapter : instance.adapters()) {
            addAdapter(adapter, format);
        }
    }
};

void render::saveAdapterInfo(const Instance& instance, DXGI_FORMAT format, db::Connection& connection) {
    connection.createTable(renderdao::Rotation::table());
    connection.createTable(renderdao::Adapter::table());
    connection.createTable(renderdao::Output::table());
    connection.createTable(renderdao::Mode::table());

    SaveAdapterInfo save{connection};
    save.submitInfo(instance, format);
}
