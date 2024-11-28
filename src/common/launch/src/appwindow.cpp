#include "launch/appwindow.hpp"

#include "archive/archive.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx12.h>

#include "archive.dao.hpp"

using namespace sm;
using namespace sm::launch;

static constexpr math::int2 kMinWindowSize = {128, 128};

extern IMGUI_IMPL_API LRESULT
ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg,
    WPARAM wParam, LPARAM lParam
);

static render::next::SurfaceInfo newSurfaceInfo(math::uint2 size) {
    return render::next::SurfaceInfo {
        .format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .size = size,
        .length = 2,
        .clear = math::float4(0.0f, 0.0f, 0.0f, 1.0f),
    };
}

static system::WindowConfig newWindowConfig(const char *title) {
    return system::WindowConfig {
        .mode = system::WindowMode::eWindowed,
        .width = 800,
        .height = 600,
        .title = title,
    };
}

static void saveWindowPlacement(db::Connection& db, WINDOWPLACEMENT placement) {
    if (db::DbError error = db.tryInsert(archive::fromWindowPlacement(placement))) {
        LOG_WARN(GlobalLog, "failed to save window placement: {}", error);
    }
}

static std::optional<WINDOWPLACEMENT> loadWindowPlacement(db::Connection& db) {
    auto result = db.trySelectOne<sm::dao::archive::WindowPlacement>();
    if (!result)
        return std::nullopt;

    WINDOWPLACEMENT placement = archive::toWindowPlacement(result.value());

    if (LONG width = placement.rcNormalPosition.right - placement.rcNormalPosition.left; width < kMinWindowSize.width) {
        LOG_WARN(GlobalLog, "window placement width too small {}, ignoring possibly corrupted data", width);
        return std::nullopt;
    }

    if (LONG height = placement.rcNormalPosition.bottom - placement.rcNormalPosition.top; height < kMinWindowSize.height) {
        LOG_WARN(GlobalLog, "window placement height too small {}, ignoring possibly corrupted data", height);
        return std::nullopt;
    }

    return placement;
}

LRESULT WindowEvents::event(system::Window& window, UINT message, WPARAM wparam, LPARAM lparam) {
    return ImGui_ImplWin32_WndProcHandler(window.getHandle(), message, wparam, lparam);
}

void WindowEvents::resize(system::Window& window, math::int2 size) {
    if (mAppWindow == nullptr)
        return;

    if (size.width < kMinWindowSize.width || size.height < kMinWindowSize.height) {
        LOG_WARN(GlobalLog, "resize too small {}/{}, ignoring", size.width, size.height);
        return;
    }

    mAppWindow->resize(newSurfaceInfo(size));
}

void AppWindow::resize(render::next::SurfaceInfo info) {
    if (mConnection)
        saveWindowPlacement(*mConnection, mWindow.getPlacement());

    getContext().updateSwapChain(info);
}

render::next::ContextConfig AppWindow::newContextConfig() {
    return render::next::ContextConfig {
        .autoSearchBehaviour = render::AdapterPreference::eMinimumPower,
        .swapChainFactory = &mSwapChainFactory,
        .swapChainInfo = newSurfaceInfo(mWindow.getClientSize()),
    };
}

void AppWindow::initWindow() {
    if (mConnection) {
        mConnection->createTable(sm::dao::archive::WindowPlacement::table());
        if (auto placement = loadWindowPlacement(*mConnection)) {
            mWindow.setPlacement(*placement);
            goto show;
        }
    }

    mWindow.centerWindow(system::MultiMonitor::ePrimary);

show:
    mWindow.showWindow(system::ShowWindow::eShow);
}

AppWindow::AppWindow(const std::string& title, db::Connection *db)
    : mConnection(db)
    , mWindow(newWindowConfig(title.c_str()), mEvents)
    , mSwapChainFactory(mWindow.getHandle())
{
    mEvents.mAppWindow = this;
}

bool AppWindow::next() {
    MSG msg = {};
    bool done = false;
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
        if (msg.message == WM_QUIT) {
            done = true;
        }
    }

    if (done) return false;

    begin();

    return true;
}

void AppWindow::present() {
    end();
}
