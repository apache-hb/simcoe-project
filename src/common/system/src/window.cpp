#include "system/window.hpp"
#include "core/format.hpp" // IWYU pragma: keep

#include "common.hpp"

#include "resource.h"

#include <winbase.h>

using namespace sm;
using namespace sm::system;

static std::string getGlfwError(const char *reason) {
    const char *description;
    int error = glfwGetError(&description);
    return fmt::format("{}: {} ({})", reason, description, error);
}

static system::WindowHandle createWindow(const system::WindowConfig& info) {
    CTASSERTF(gInstance != nullptr, "system::create() not called before Window::create()");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if (GLFWwindow *window = glfwCreateWindow(info.width, info.height, info.title.c_str(), nullptr, nullptr)) {
        return system::WindowHandle(window, glfwDestroyWindow);
    }

    throw std::runtime_error(getGlfwError("glfwCreateWindow failed"));
}

Window::Window(const WindowConfig &info, IWindowEvents& events)
    : mWindow(createWindow(info))
    , mTitle(info.title)
    , mEvents(events)
{
    glfwSetWindowUserPointer(mWindow.get(), this);

    glfwSetWindowCloseCallback(mWindow.get(), [](GLFWwindow *window) {
        Window *self = static_cast<Window*>(glfwGetWindowUserPointer(window));
        self->mEvents.close(*self);
    });

    glfwSetWindowSizeCallback(mWindow.get(), [](GLFWwindow *window, int width, int height) {
        Window *self = static_cast<Window*>(glfwGetWindowUserPointer(window));
        self->mEvents.resize(*self, { width, height });
    });

    glfwSetWindowSizeLimits(mWindow.get(), info.minSize.x, info.minSize.y, info.maxSize.x, info.maxSize.y);

    showWindow(ShowWindow::eShowNormal);
}

system::WindowPlacement Window::getPlacement(void) const {
    return system::WindowPlacement { getPosition(), getSize() };
}

void Window::setPlacement(WindowPlacement placement) {
    setPosition(placement.position);
    setSize(placement.size);
}

void Window::setPosition(math::int2 position) {
    glfwSetWindowPos(mWindow.get(), position.x, position.y);
}

math::int2 Window::getPosition() const {
    int x, y;
    glfwGetWindowPos(mWindow.get(), &x, &y);
    return { x, y };
}

math::int2 Window::getSize() const {
    int width, height;
    glfwGetWindowSize(mWindow.get(), &width, &height);
    return { width, height };
}

void Window::setSize(math::int2 size) {
    glfwSetWindowSize(mWindow.get(), size.width, size.height);
}

void Window::showWindow(ShowWindow show) {
    glfwShowWindow(mWindow.get());
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(mWindow.get());
}

void Window::pollEvents() {
    glfwPollEvents();
}

void Window::setTitle(const std::string& title) {
    mTitle = title;
    glfwSetWindowTitle(mWindow.get(), title.c_str());
}

std::string Window::getTitle() const {
    return mTitle;
}

static std::span<GLFWmonitor*> getMonitors() {
    int count;
    GLFWmonitor **monitors = glfwGetMonitors(&count);
    return {monitors, static_cast<size_t>(count)};
}

static system::WindowPlacement getMonitorPlacement(GLFWmonitor *monitor) {
    int x, y, w, h;
    glfwGetMonitorWorkarea(monitor, &x, &y, &w, &h);

    return system::WindowPlacement { { x, y }, { w, h } };
}

static void centerOnPrimaryMonitor(Window& window) {
    system::WindowPlacement placement = getMonitorPlacement(glfwGetPrimaryMonitor());

    math::int2 size = window.getSize();

    window.setPosition((size - placement.size) / 2);
}

static GLFWmonitor *getNearestMonitor(math::int2 center) {
    auto distanceTo = [&](system::WindowPlacement placement) -> int {
        return math::distance(placement.center(), center);
    };

    std::span<GLFWmonitor*> monitors = getMonitors();
    GLFWmonitor *closest = glfwGetPrimaryMonitor();
    int bestDistance = distanceTo(getMonitorPlacement(closest));
    for (GLFWmonitor *monitor : monitors) {
        int dist = distanceTo(getMonitorPlacement(monitor));
        if (dist < bestDistance) {
            bestDistance = dist;
            closest = monitor;
        }
    }

    return closest;
}

static void centerOnNearestMonitor(Window& window) {
    system::WindowPlacement placement = window.getPlacement();
    GLFWmonitor *nearest = getNearestMonitor(placement.center());

    system::WindowPlacement monitor = getMonitorPlacement(nearest);

    // center the window on the monitor
    math::int2 monitorPosition = monitor.position;
    math::int2 monitorSize = monitor.size;
    math::int2 windowSize = placement.size;

    int x = monitorPosition.x + (monitorSize.width - windowSize.width) / 2;
    int y = monitorPosition.y + (monitorSize.height - windowSize.height) / 2;

    window.setPosition(math::int2{ x, y });
}

void Window::centerWindow(MultiMonitor monitor, bool topmost) {
    switch (monitor) {
    case MultiMonitor::ePrimary:
        centerOnPrimaryMonitor(*this);
        break;
    case MultiMonitor::eNearest:
        centerOnNearestMonitor(*this);
        break;
    }

    if (topmost) {
        glfwFocusWindow(mWindow.get());
    }
}

uint64_t system::saveWindowInfo(db::Connection& db, const Window& window) {
    math::int2 size = window.getSize();
    math::int2 position = window.getPosition();

    dao::system::WindowInfo info {
        .title = window.getTitle(),
        .x = position.x,
        .y = position.y,
        .width = size.width,
        .height = size.height
    };

    return db.insertReturningPrimaryKey(info);
}

std::optional<dao::system::WindowInfo> system::loadWindowInfo(db::Connection& db, uint64_t id) try {
    return db.selectByPrimaryKey<dao::system::WindowInfo>(id);
} catch (const db::DbException& e) {
    LOG_WARN(SystemLog, "failed to load window info: {}", e.error());
    return std::nullopt;
}

void system::applyWindowInfo(Window& window, const dao::system::WindowInfo& info) {
    window.setTitle(info.title);
    window.setPosition({ info.x, info.y });
    window.setSize({ info.width, info.height });
}
