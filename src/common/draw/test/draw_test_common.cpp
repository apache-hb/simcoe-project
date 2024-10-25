#include "draw_test_common.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx12.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam,
                                                             LPARAM lParam);

LRESULT ImGuiWindowEvents::event(system::Window& window, UINT message, WPARAM wparam, LPARAM lparam) {
    return ImGui_ImplWin32_WndProcHandler(window.getHandle(), message, wparam, lparam);
}
