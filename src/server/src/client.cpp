#include "imgui/imgui.h"
#include "stdafx.hpp"
#include "common.hpp"
#include "db/connection.hpp"

#include "threads/mailbox.hpp"

#include "net/net.hpp"

#include "launch/launch.hpp"
#include "account/account.hpp"

#include <imgui/misc/imgui_stdlib.h>
#include <blockingconcurrentqueue.h>

using namespace std::chrono_literals;

LOG_MESSAGE_CATEGORY(ClientLog, "Client");

static const launch::LaunchInfo kLaunchInfo {
    .logDbType = db::DbType::eSqlite3,
    .logDbConfig = { .host = "client-logs.db" },
    .logPath = "client.log",

    .infoDbConfig = { .host = "client.db" },

    .network = true,
    .glfw = true,
};

enum ClientState {
    eDisconnected,
    eConnecting,
    eConnected,
    eConnectError,
    eLoggingIn,
    eLoginError,
    eLoggedIn,
};

using EventQueue = moodycamel::BlockingConcurrentQueue<std::function<void()>>;

namespace ImGui
{
    void Spinner(const char *title, float frequency = 0.05f)
    {
        char symbol = "|/-\\"[(int)(ImGui::GetTime() / frequency) & 3];

        ImGui::Text("%s %c", title, symbol);
    }
}

struct MessageListWidget {
    threads::NonBlockingMailBox<std::vector<game::Message>> messages;
    std::string message;

    std::atomic<bool> updating = false;

    float lastUpdate = 0;
    float updateRate = 1.f;

    void refreshMessages(EventQueue& events, game::AccountClient& client) {
        events.enqueue([&] {
            if (updating)
                return;

            try {
                // very bad rate limiting
                updating = true;
                defer { updating = false; };

                client.work();
                client.refreshMessageList();
                messages.write(auto(client.getMessages()));
            } catch (const std::exception& e) {
                LOG_ERROR(ClientLog, "Failed to refresh messages: {}", e.what());
            }
        });
    }

    void draw(EventQueue& events, game::AccountClient& client) {
        if (ImGui::Button("Refresh##Messages")) {
            refreshMessages(events, client);
        }

        // float now = ImGui::GetTime();
        // if (now - lastUpdate > updateRate) {
        //     refreshMessages(events, client);
        //     lastUpdate = now;
        // }

        if (ImGui::BeginTable("Messages", 2)) {
            ImGui::TableSetupColumn("Author", ImGuiTableColumnFlags_WidthFixed, 100);
            ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableHeadersRow();

            std::lock_guard guard(messages);
            for (const auto& message : messages.read()) {
                ImGui::TableNextRow();

                ImGui::NextColumn();
                ImGui::TextUnformatted(message.author.c_str());
                ImGui::NextColumn();
                ImGui::TextUnformatted(message.message.c_str());
            }

            ImGui::EndTable();
        }

        if (ImGui::InputText("Send", &message, ImGuiInputTextFlags_EnterReturnsTrue)) {
            events.enqueue([&, message = message] {
                try {
                    client.sendMessage(message);
                    messages.write(auto(client.getMessages()));
                    client.work();
                } catch (const std::exception& e) {
                    LOG_ERROR(ClientLog, "Failed to send message: {}", e.what());
                }
            });

            message.clear();
        }
    }
};

struct LobbyListWidget {
    enum State {
        eIdle,
        eUpdating,
        eDoneUpdate,
        eError,
    };

    std::atomic<State> state = eIdle;
    std::string error = "";

    threads::NonBlockingMailBox<std::vector<game::LobbyInfo>> lobbies;

    void drawTable() {
        ImGui::SeparatorText("Lobbies");

        if (ImGui::BeginTable("Lobbies", 4)) {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 100);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Host", ImGuiTableColumnFlags_WidthFixed, 100);
            ImGui::TableSetupColumn("Players", ImGuiTableColumnFlags_WidthFixed, 100);

            ImGui::TableHeadersRow();

            std::lock_guard guard(lobbies);

            for (const auto& session : lobbies.read()) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%" PRIu64, session.id);
                ImGui::TableNextColumn();
                std::string_view name = session.name.text();
                ImGui::Text("%.*s", (int)name.size(), name.data());
                ImGui::TableNextColumn();
                game::SessionId host = session.getHost();
                ImGui::Text("%" PRIu64, host);
                ImGui::TableNextColumn();
                ImGui::Text("%d", session.getPlayerCount());
            }

            ImGui::EndTable();
        }
    }

    void refreshList(EventQueue& events, game::AccountClient& client) {
        events.enqueue([&] {
            try {
                state.store(eUpdating);
                client.refreshLobbyList();
                lobbies.write(client.getLobbyInfo());
                state.store(eDoneUpdate);
            } catch (const std::exception& e) {
                error = e.what();
                state.store(eError);
            }
        });
    }

    void refreshButton(EventQueue& events, game::AccountClient& client) {
        if (ImGui::Button("Refresh##Lobbies")) {
            refreshList(events, client);
        }
    }

    void draw(EventQueue& events, game::AccountClient& client) {
        State s = state.load();

        if (s == eError) {
            ImGui::Text("Error: %s", error.c_str());
            refreshButton(events, client);
        } else if (s == eIdle || s == eDoneUpdate) {
            drawTable();
            refreshButton(events, client);
        } else if (s == eUpdating) {
            drawTable();
            ImGui::Spinner("Updating");
        }
    }
};

struct SessionListWidget {
    enum State {
        eIdle,
        eUpdating,
        eDoneUpdate,
        eError,
    };

    std::atomic<State> state = eIdle;
    std::string error = "";

    threads::NonBlockingMailBox<std::vector<game::SessionInfo>> sessions;

    void drawTable() {
        ImGui::SeparatorText("Sessions");

        if (ImGui::BeginTable("Sessions", 2)) {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 100);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableHeadersRow();

            std::lock_guard guard(sessions);

            for (const auto& session : sessions.read()) {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text("%" PRIu64, session.id);

                ImGui::TableNextColumn();
                std::string_view name = session.name.text();
                ImGui::Text("%.*s", (int)name.size(), name.data());
            }

            ImGui::EndTable();
        }
    }

    void refreshList(EventQueue& events, game::AccountClient& client) {
        events.enqueue([&] {
            try {
                state.store(eUpdating);
                client.refreshSessionList();
                sessions.write(client.getSessionInfo());
                state.store(eDoneUpdate);
            } catch (const std::exception& e) {
                error = e.what();
                state.store(eError);
            }
        });
    }

    void refreshButton(EventQueue& events, game::AccountClient& client) {
        if (ImGui::Button("Refresh##Sessions")) {
            refreshList(events, client);
        }
    }

    void draw(EventQueue& events, game::AccountClient& client) {
        State s = state.load();

        if (s == eError) {
            ImGui::Text("Error: %s", error.c_str());
            refreshButton(events, client);
        } else if (s == eIdle || s == eDoneUpdate) {
            drawTable();
            refreshButton(events, client);
        } else if (s == eUpdating) {
            drawTable();
            ImGui::Spinner("Updating");
        }
    }
};

static int commonMain(launch::LaunchResult&) noexcept try {
    net::Network network = net::Network::create();
    EventQueue events;
    moodycamel::ConcurrentQueue<std::function<void()>> responses;

    std::atomic<ClientState> state = eDisconnected;
    std::unique_ptr<game::AccountClient> client;

    std::jthread clientThread = std::jthread([&](const std::stop_token& stop) {
        while (!stop.stop_requested()) {
            std::function<void()> event;

            LOG_INFO(GlobalLog, "Waiting for event");
            
            events.wait_dequeue(event);
            
            LOG_INFO(GlobalLog, "New event");

            event();

            LOG_INFO(GlobalLog, "Event executed");

            // if (client) {
            //     client->work();
            // }
        }
    });

    char username[32] = "";
    char password[32] = "";
    char error[256] = "";
    char lobby[32] = "";

    char host[64] = "127.0.0.1";
    unsigned port = 9919;

    SessionListWidget sessionList;
    LobbyListWidget lobbyList;
    MessageListWidget messages;

    auto connectToServerAsync = [&] {
        state.store(eConnecting);
        events.enqueue([&, host = net::Address::of(host), port = port] {
            try {
                client = std::make_unique<game::AccountClient>(network, host, port);
                state.store(eConnected);
            } catch (const std::exception& err) {
                std::strncpy(error, err.what(), sizeof(error));
                state.store(eConnectError);
            }
        });
    };

    launch::GuiWindow window{"Client"};
    while (!window.shouldClose()) {
        window.begin();

        ImGui::ShowDemoWindow();

        if (ImGui::Begin("Client")) {
            ClientState current = state.load();
            if (current == eDisconnected || current == eConnecting || current == eConnectError) {
                ImGui::BeginDisabled(current == eConnecting);

                ImGui::InputText("Host", host, sizeof(host));
                ImGui::InputScalar("Port", ImGuiDataType_U16, &port);

                if (ImGui::Button("Connect")) {
                    connectToServerAsync();
                }

                if (current == eConnecting) {
                    ImGui::Spinner("Connecting");
                } else if (current == eConnectError) {
                    ImGui::Text("Failed to connect: %s", error);
                } else if (current == eDisconnected) {
                    ImGui::Text("Disconnected");
                }

                ImGui::EndDisabled();
            } else if (current == eConnected || current == eLoggingIn || current == eLoginError) {
                ImGui::BeginDisabled(current == eLoggingIn);

                ImGui::InputText("Username", username, sizeof(username));
                ImGui::InputText("Password", password, sizeof(password), ImGuiInputTextFlags_Password);

                if (ImGui::Button("Login")) {
                    state.store(eLoggingIn);
                    events.enqueue([&, username = username, password = password] {
                        try {
                            if (client->login(username, password)) {
                                state.store(eLoggedIn);
                            } else {
                                std::strncpy(error, "Login failed", sizeof(error));
                                state.store(eLoginError);
                            }
                        } catch (const net::NetException& err) {
                            std::strncpy(error, err.what(), sizeof(error));
                            state.store(eLoginError);
                        }

                        if (state.load() == eLoggedIn) {
                            sessionList.refreshList(events, *client);
                            lobbyList.refreshList(events, *client);
                        }
                    });
                }

                ImGui::SameLine();

                if (ImGui::Button("Create Account")) {
                    state.store(eLoggingIn);
                    events.enqueue([&, username = username, password = password] {
                        try {
                            if (client->createAccount(username, password)) {
                                if (!client->login(username, password)) {
                                    std::strncpy(error, "Login failed", sizeof(error));
                                    state.store(eLoginError);
                                } else {
                                    state.store(eLoggedIn);
                                }
                            } else {
                                std::strncpy(error, "Account creation failed", sizeof(error));
                                state.store(eLoginError);
                            }
                        } catch (const std::exception& err) {
                            std::strncpy(error, err.what(), sizeof(error));
                            state.store(eLoginError);
                        }

                        if (state.load() == eLoggedIn) {
                            sessionList.refreshList(events, *client);
                            lobbyList.refreshList(events, *client);
                        }
                    });
                }

                ImGui::EndDisabled();

                if (current == eLoggingIn) {
                    ImGui::Spinner("Authenticating");
                } else if (current == eLoginError) {
                    ImGui::Text("Failed to login: %s", error);
                }
            } else if (current == eLoggedIn) {
                ImGui::Text("Logged in as %s", username);

                sessionList.draw(events, *client);
                lobbyList.draw(events, *client);

                if (ImGui::Button("Logout")) {
                    state.store(eDisconnected);
                    client.reset();
                    connectToServerAsync();
                }

                ImGui::SameLine();

                if (ImGui::Button("Disconnect")) {
                    state.store(eDisconnected);
                    client.reset();
                }

                ImGui::SameLine();

                if (ImGui::Button("Create Lobby")) {
                    ImGui::OpenPopup("Create Lobby");
                }

                if (ImGui::BeginPopup("Create Lobby")) {
                    ImGui::InputText("Name", lobby, sizeof(lobby));

                    if (ImGui::Button("Create")) {
                        events.enqueue([&, lobby = lobby] {
                            try {
                                client->createLobby(lobby);
                                lobbyList.refreshList(events, *client);
                            } catch (const std::exception& e) {
                                std::strncpy(error, e.what(), sizeof(error));
                            }
                        });

                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }
            }
        }
        ImGui::End();

        if (state == eLoggedIn) {
            if (ImGui::Begin("Messages")) {
                messages.draw(events, *client);
            }
            ImGui::End();
        }

        window.end();
    }

    clientThread.request_stop();
    events.enqueue([] {});

    return 0;
} catch (const errors::AnyException& err) {
    LOG_ERROR(GlobalLog, "{}", err.what());
    for (const auto& frame : err.stacktrace()) {
        LOG_ERROR(GlobalLog, "{}:{} - {}", frame.source_file(), frame.source_line(), frame.description());
    }
    return -1;
} catch (const std::exception& err) {
    LOG_ERROR(GlobalLog, "{}", err.what());
    return -1;
} catch (...) {
    LOG_ERROR(GlobalLog, "Unknown unhandled exception.");
    return -1;
}

SM_LAUNCH_MAIN("Client", commonMain, kLaunchInfo)
