#include "contextmenu/contextmenu.hpp"

#include "db/environment.hpp"
#include "db/connection.hpp"

#include "launch/launch.hpp"
#include "launch/appwindow.hpp"

#include "core/error/error.hpp"

#include <imgui/imgui.h>

using namespace sm;

namespace cm = sm::windows::contextmenu;

LOG_MESSAGE_CATEGORY(ProgramLog, "Program");

class GuiWindow : public launch::AppWindow {
    draw::next::DrawContext mContext;

    void begin() override { mContext.begin(); }
    void end() override { mContext.end(); }

    render::next::CoreContext& getContext() override { return mContext; }

public:
    GuiWindow(const std::string& title)
        : AppWindow(title)
        , mContext(newContextConfig(), mWindow.getHandle())
    {
        initWindow();
    }
};

static void drawClassTable(const char *id, std::span<const cm::FileClass> classes) {
    if (ImGui::TreeNode(id)) {
        static constexpr ImGuiTableFlags kTableFlags
            = ImGuiTableFlags_Resizable
            | ImGuiTableFlags_Borders;

        if (ImGui::BeginTable("Classes", 2, kTableFlags)) {
            ImGui::TableSetupColumn("Extension", ImGuiTableColumnFlags_None);
            ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_None);

            ImGui::TableHeadersRow();

            for (const auto& fileClass : classes) {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text("%s", fileClass.extension.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("Description");
            }

            ImGui::EndTable();
        }
        ImGui::TreePop();
    }
}

static int commonMain() noexcept try {
    cm::ContextMenuRegistry registry;

    db::Environment env = db::Environment::create(db::DbType::eSqlite3);
    db::Connection db = env.connect({ .host = "contextmenu.db" });
    registry.save(db);

    GuiWindow window{"Context Menu Editor"};
    while (window.next()) {
        ImGui::ShowDemoWindow();

        if (ImGui::Begin("Context Menu Editor")) {
            drawClassTable("HKEY_CLASSES_ROOT\\", registry.global.classes);
            drawClassTable("HKEY_CURRENT_USER\\Software\\", registry.user.classes);
        }
        ImGui::End();

        window.present();
    }

    return 0;
} catch(const errors::AnyException& err) {
    LOG_ERROR(ProgramLog, "unhandled exception: {}", err.what());
    for (const auto& frame : err.stacktrace()) {
        LOG_ERROR(ProgramLog, "  {} ({}:{})", frame.description(), frame.source_file(), frame.source_line());
    }
    return -1;
} catch (const std::exception& err) {
    LOG_ERROR(ProgramLog, "unhandled exception: {}", err.what());
    return -1;
} catch (...) {
    LOG_ERROR(ProgramLog, "unknown unhandled exception");
    return -1;
}

static const launch::LaunchInfo kLaunchInfo {
    .logDbType = db::DbType::eSqlite3,
    .logDbConfig = { .host = "test-contextmenu-logs.db" },
    .logPath = "test-contextmenu.log",

    .threads = false,
    .network = false,
    .com = false,
};

int main(int argc, const char **argv) noexcept {
    launch::LaunchResult launch = launch::commonInitMain(argc, argv, kLaunchInfo);
    if (launch.shouldExit()) {
        return launch.exitCode();
    }

    int result = commonMain();

    LOG_INFO(ProgramLog, "editor exiting with {}", result);

    return result;
}

int WinMain(HINSTANCE hInstance, SM_UNUSED HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    launch::LaunchResult launch = launch::commonInitWinMain(hInstance, nShowCmd, kLaunchInfo);
    if (launch.shouldExit()) {
        return launch.exitCode();
    }

    int result = commonMain();

    LOG_INFO(ProgramLog, "editor exiting with {}", result);

    return result;
}
