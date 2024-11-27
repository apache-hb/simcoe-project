#include "launch/appwindow.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_dx12.h>

#include "archive.dao.hpp"

using namespace sm;
using namespace sm::launch;

static constexpr math::int2 kMinWindowSize = {128, 128};

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
    if (mConnection) {
        system::saveWindowInfo(*mConnection, mWindow);
    }

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
        mConnection->createTable(dao::system::WindowInfo::table());
        if (std::optional placement = system::loadWindowInfo(*mConnection, 0)) {
            system::applyWindowInfo(mWindow, *placement);
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
    bool done = mWindow.shouldClose();
    if (!done) {
        mWindow.pollEvents();
        begin();
    }
    return !done;
}

void AppWindow::present() {
    end();
}
