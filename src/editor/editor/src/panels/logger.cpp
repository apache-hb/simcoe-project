#include "stdafx.hpp"

#include "editor/panels/logger.hpp"

using namespace sm;
using namespace sm::ed;

LoggerPanel LoggerPanel::gInstance{};

LoggerPanel& LoggerPanel::get() {
    return gInstance;
}

LoggerPanel::LoggerPanel() {
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

    mMessages[&message.category].push_back(msg);
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

void LoggerPanel::draw_category(const logs::LogCategory& category) const {
    const LogMessages &messages = mMessages.at(&category);

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

void LoggerPanel::draw_window() {
    if (!mOpen) return;

    if (ImGui::Begin("Logs", &mOpen)) {
        if (ImGui::BeginTabBar("Logs")) {
            for (const auto& [category, messages] : mMessages) {
                if (messages.empty()) continue;

                if (ImGui::BeginTabItem(category->name().data())) {
                    draw_category(*category);
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}
