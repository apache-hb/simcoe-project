#include "render/editor/logger.hpp"

#include "imgui/imgui.h"

using namespace sm;
using namespace sm::editor;

LoggerPanel LoggerPanel::gInstance{};

LoggerPanel& LoggerPanel::get() {
    return gInstance;
}

LoggerPanel::LoggerPanel() noexcept
    : IEditorPanel("Logs")
{
    auto& instance = logs::get_logger();
    instance.add_channel(this);
}

LoggerPanel::~LoggerPanel() {
    auto& instance = logs::get_logger();
    instance.remove_channel(this);
}

void LoggerPanel::accept(const logs::Message &message) {
    Message msg = {
        .severity = message.severity,
        .timestamp = message.timestamp,
        .thread = message.thread,
        .message = sm::String{message.message}
    };

    mMessages[message.category.as_integral()].push_back(msg);
}

static ImVec4 get_severity_colour(logs::Severity severity) {
    using enum logs::Severity::Inner;
    switch (severity) {
        // blue
    case eTrace: return ImVec4{0.0f, 0.0f, 1.0f, 1.0f};

        // green
    case eInfo: return ImVec4{0.0f, 1.0f, 0.0f, 1.0f};

        // yellow
    case eWarning: return ImVec4{1.0f, 1.0f, 0.0f, 1.0f};

        // orange
    case eError: return ImVec4{1.0f, 0.5f, 0.0f, 1.0f};

        // red
    case eFatal: return ImVec4{1.0f, 0.0f, 0.0f, 1.0f};

        // cyan
    case ePanic: return ImVec4{0.0f, 1.0f, 1.0f, 1.0f};

    default: return ImVec4{1.0f, 1.0f, 1.0f, 1.0f};
    }
}

constexpr static ImGuiTableFlags kFlags
    = ImGuiTableFlags_Resizable
    | ImGuiTableFlags_Hideable
    | ImGuiTableFlags_RowBg
    | ImGuiTableFlags_ScrollY;

using ReflectCategory = ctu::TypeInfo<logs::Category>;
using ReflectSeverity = ctu::TypeInfo<logs::Severity>;

void LoggerPanel::draw_category(logs::Category category) const {
    const LogMessages &messages = mMessages[category.as_integral()];

    if (ImGui::BeginTable("Messages", 3, kFlags)) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Severity", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Timestamp", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (const Message &message : messages) {
            ImVec4 colour = get_severity_colour(message.severity);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(colour, "%s", ReflectSeverity::to_string(message.severity).c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%u", message.timestamp);
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(message.message.data());
        }

        ImGui::EndTable();
    }
}

void LoggerPanel::draw_content() {
    if (ImGui::BeginTabBar("Logs")) {
        for (size_t i = 0; i < kCategoryCount; ++i) {
            logs::Category category = (logs::Category)i;
            const LogMessages &messages = mMessages[i];

            ImGui::BeginDisabled(messages.empty());

            using ReflectCategory = ctu::TypeInfo<logs::Category>;

            if (ImGui::BeginTabItem(ReflectCategory::to_string(category).c_str())) {
                draw_category(category);
                ImGui::EndTabItem();
            }

            ImGui::EndDisabled();
        }
        ImGui::EndTabBar();
    }
}
