#pragma once

#include "system/system.hpp" // IWYU pragma: export

namespace sm::sys {
    class SplashScreen : public IWindowEvents {
        void paint(Window& window) override;

        HBITMAP m_splash;
        int m_progress = 0; // 0 - 100
        std::string m_status;

        sys::Window m_window;
        logs::Sink<logs::Category::eSystem> m_log;

    public:
        SplashScreen(const SplashConfig& config);
        ~SplashScreen();

        void set_status_text(const char *text);
        void set_progress(int progress);
    };
}
