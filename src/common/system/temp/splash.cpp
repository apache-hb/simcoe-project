#include "system/splash.hpp"
#include "common.hpp"

#include "resource.h"

#include <wingdi.h>

using namespace sm;
using namespace sm::sys;

void SplashScreen::paint(Window& window) {
    WindowCoords rect = window.get_coords();
    if (rect.bottom == 0) return;
    PAINTSTRUCT ps{};

    HWND hwnd = window.get_handle();
    HDC hdc = BeginPaint(hwnd, &ps);

    HDC memdc = CreateCompatibleDC(hdc);

    HGDIOBJ old = SelectObject(memdc, m_splash);
    SM_ASSERT_WIN32(BitBlt(hdc, 0, 0, rect.right, rect.bottom, memdc, 0, 0, SRCCOPY));

    // draw progress bar
    if (m_progress > 0) {
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
        SM_ASSERT_WIN32(pen != nullptr);

        HGDIOBJ oldpen = SelectObject(memdc, pen);
        SM_ASSERT_WIN32(oldpen != nullptr);
        int width = rect.right - rect.left;
        // int height = rect.bottom - rect.top;
        int progress = width * m_progress / 100;

        printf("progress: %d\n", progress);

        HBRUSH brush = CreateSolidBrush(RGB(255, 255, 0x00));
        // draw progress
        RECT area{ 0, rect.bottom - 30, progress, rect.bottom };
        SM_ASSERT_WIN32(brush != nullptr);
        SM_ASSERT_WIN32(Rectangle(memdc, area.left, area.top, area.right, area.bottom));
        SM_ASSERT_WIN32(DeleteObject(brush));
    }

    SelectObject(memdc, old);
    DeleteDC(memdc);

    EndPaint(hwnd, &ps);
}

SplashScreen::SplashScreen(const SplashConfig& config)
    : m_window(WindowConfig {
        .mode = sys::WindowMode::eBorderless,
        .width = config.width,
        .height = config.height,
        .title = "Splash Screen",
        .logger = config.logger,
    }, this)
    , m_log(config.logger)
{
    m_splash = LoadBitmapA(gInstance, MAKEINTRESOURCEA(IDB_SPLASH));
    SM_ASSERT_WIN32(m_splash != nullptr);
    m_log.info("splash screen loaded");

    m_progress = 50;

    m_window.show_window(sys::ShowWindow::eShow);
    m_window.center_window(MultiMonitor{}, true);
}

SplashScreen::~SplashScreen() {
    SM_ASSERT_WIN32(DeleteObject(m_splash));
}

void SplashScreen::set_status_text(const char *text) {
    m_status = text;

    // invalidate window
    SM_ASSERT_WIN32(InvalidateRect(m_window.get_handle(), nullptr, FALSE));
}

void SplashScreen::set_progress(int progress) {
    m_progress = progress;

    // invalidate window
    SM_ASSERT_WIN32(InvalidateRect(m_window.get_handle(), nullptr, FALSE));
}
