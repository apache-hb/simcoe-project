#include "common.hpp"

using namespace sm::launch;

GuiWindow::GuiWindow(const std::string& title)
    : AppWindow(title)
    , mContext(newContextConfig(), mWindow.getHandle())
{
    initWindow();
}

void GuiWindow::begin() {
    mContext.begin();
}

void GuiWindow::end() {
    mContext.end();
}
