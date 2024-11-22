#include "launch/guiwindow.hpp"
#include "core/error.hpp"
#include "core/error/error.hpp"

#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "math/colour.hpp"

using namespace sm;
using namespace sm::launch;

static void throwGlfwError() {
    const char* description;
    int error = glfwGetError(&description);
    throw OsException{OsError{os_error_t(error), "GLFW: {}", description}};
}

static GLFWwindow *createWindow(const char *title) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow *window = glfwCreateWindow(800, 600, title, nullptr, nullptr);
    if (window == nullptr) {
        throwGlfwError();
    }

    return window;
}

GuiWindow::GuiWindow(const std::string& title)
    : mWindow(createWindow(title.c_str()))
{
    glfwMakeContextCurrent(mWindow);
    glfwSwapInterval(1);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplGlfw_InitForOpenGL(mWindow, true);
    ImGui_ImplOpenGL3_Init("#version 130");
}

GuiWindow::~GuiWindow() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(mWindow);
}

void GuiWindow::begin() {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void GuiWindow::end() {
    math::float4 clear = math::float4 { 0.45f, 0.55f, 0.60f, 1.00f };
    ImGuiIO& io = ImGui::GetIO();

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(mWindow, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear.x * clear.w, clear.y * clear.w, clear.z * clear.w, clear.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(mWindow);
    }

    glfwSwapBuffers(mWindow);
}

bool GuiWindow::shouldClose() const {
    return glfwWindowShouldClose(mWindow);
}
