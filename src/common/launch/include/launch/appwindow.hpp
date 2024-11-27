#pragma once

#include "db/environment.hpp"

#include "draw/next/context.hpp"
#include "render/next/surface/swapchain.hpp"

namespace sm::launch {
    class AppWindow;
    class WindowEvents;

    class WindowEvents : public system::IWindowEvents {
        friend AppWindow;

        AppWindow *mAppWindow;

        void resize(system::Window& window, math::int2 size) override;
    };

    class AppWindow {
        friend WindowEvents;

        virtual void begin() = 0;
        virtual void end() = 0;
        virtual render::next::CoreContext& getContext() = 0;

        void resize(render::next::SurfaceInfo info);

    protected:
        db::Connection *mConnection;
        WindowEvents mEvents;
        system::Window mWindow;
        render::next::WindowSwapChainFactory mSwapChainFactory;

        void initWindow();
        render::next::ContextConfig newContextConfig();

    public:
        virtual ~AppWindow() = default;
        AppWindow(const std::string& title, db::Connection *db = nullptr);

        bool next();
        void present();
    };
}
