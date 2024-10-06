#include "stdafx.hpp"

#include "editor/panels/logger.hpp"

using namespace sm;
using namespace sm::ed;

LoggerPanel LoggerPanel::gInstance{};

LoggerPanel& LoggerPanel::get() {
    return gInstance;
}

LoggerPanel::LoggerPanel() {
    // auto& instance = structured::Logger::instance();
    // instance.addChannel(*this);
}

LoggerPanel::~LoggerPanel() {
    // auto& instance = logs::getGlobalLogger();
    // instance.removeChannel(*this);
}

void LoggerPanel::postMessage(structured::MessagePacket packet) noexcept {
    Message msg = {
        .severity = packet.message.level,
        .timestamp = uint32_t(packet.timestamp),
        // .thread = message.thread,
        .message = fmt::vformat(packet.message.message, *packet.args)
    };

    mMessages[&packet.message.category].push_back(msg);
}

static ImVec4 getSeverityColour(logs::Severity severity) {
    using enum logs::Severity;
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

static constexpr ImGuiTableFlags kFlags
    = ImGuiTableFlags_Resizable
    | ImGuiTableFlags_Hideable
    | ImGuiTableFlags_RowBg
    | ImGuiTableFlags_ScrollY;

using ReflectSeverity = ctu::TypeInfo<logs::Severity>;

void LoggerPanel::drawLogCategory(const structured::CategoryInfo& category) const {
    const LogMessages &messages = mMessages.at(&category);

    if (ImGui::BeginTable("Messages", 3, kFlags)) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Severity", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Timestamp", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (const Message &message : messages) {
            ImVec4 colour = getSeverityColour(message.severity);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(colour, "%s", logs::toString(message.severity).data());
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

                if (ImGui::BeginTabItem(category->name.data())) {
                    drawLogCategory(*category);
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}
