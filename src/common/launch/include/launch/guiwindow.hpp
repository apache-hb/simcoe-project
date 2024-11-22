#pragma once

#include "core/macros.hpp"
#include <GLFW/glfw3.h>

#include <string>

#include <imgui/imgui.h>

namespace sm::launch {
    class GuiWindow {
        GLFWwindow *mWindow = nullptr;
    public:
        GuiWindow(const std::string& title);
        ~GuiWindow();

        SM_NOCOPY(GuiWindow);
        SM_NOMOVE(GuiWindow);

        void begin();
        void end();

        bool shouldClose() const;
    };
}
