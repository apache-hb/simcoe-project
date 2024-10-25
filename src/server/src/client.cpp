#include "stdafx.hpp"
#include "common.hpp"

#include "db/connection.hpp"

#include "net/net.hpp"

#include "launch/launch.hpp"
#include "account/account.hpp"

#include <blockingconcurrentqueue.h>

LOG_MESSAGE_CATEGORY(ClientLog, "Client");

static const launch::LaunchInfo kLaunchInfo {
    .logDbType = db::DbType::eSqlite3,
    .logDbConfig = { .host = "client-logs.db" },
    .logPath = "client.log",

    .network = true,
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

static int commonMain() noexcept try {
    net::Network network = net::Network::create();
    moodycamel::BlockingConcurrentQueue<std::function<void()>> events;

    std::atomic<ClientState> state = eDisconnected;
    std::unique_ptr<game::AccountClient> client;

    std::jthread clientThread = std::jthread([&](const std::stop_token& stop) {
        while (!stop.stop_requested()) {
            std::function<void()> event;
            events.wait_dequeue(event);

            event();
        }
    });

    char username[32] = "";
    char password[32] = "";
    char error[256] = "";

    char host[64] = "127.0.0.1";
    unsigned port = 9919;

    auto connectToServerAsync = [&] {
        state.store(eConnecting);
        events.enqueue([&, host = net::Address::of(host), port = port] {
            try {
                client = std::make_unique<game::AccountClient>(network, host, port);
                state.store(eConnected);
            } catch (const net::NetException& err) {
                std::strncpy(error, err.what(), sizeof(error));
                state.store(eConnectError);
            }
        });
    };

    GuiWindow window{"Client"};
    while (window.next()) {
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
                    ImGui::Text("Connecting %c", "|/-\\"[(int)(ImGui::GetTime() / 0.05f) & 3]);
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
                    });
                }

                ImGui::SameLine();

                if (ImGui::Button("Create Account")) {
                    state.store(eLoggingIn);
                    events.enqueue([&, username = username, password = password] {
                        try {
                            if (client->createAccount(username, password)) {
                                state.store(eLoggedIn);
                            } else {
                                std::strncpy(error, "Account creation failed", sizeof(error));
                                state.store(eLoginError);
                            }
                        } catch (const net::NetException& err) {
                            std::strncpy(error, err.what(), sizeof(error));
                            state.store(eLoginError);
                        }
                    });
                }

                ImGui::EndDisabled();

                if (current == eLoggingIn) {
                    ImGui::Text("Logging in %c", "|/-\\"[(int)(ImGui::GetTime() / 0.05f) & 3]);
                } else if (current == eLoginError) {
                    ImGui::Text("Failed to login: %s", error);
                }
            } else if (current == eLoggedIn) {
                ImGui::Text("Logged in as %s", username);

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
            }
        }
        ImGui::End();

        window.present();
    }

    clientThread.request_stop();
    events.enqueue([] {});

    return 0;
} catch (const std::exception& err) {
    LOG_ERROR(ClientLog, "unhandled exception: {}", err.what());
    return -1;
} catch (...) {
    LOG_ERROR(ClientLog, "unknown unhandled exception");
    return -1;
}

int main(int argc, const char **argv) noexcept try {
    auto _ = launch::commonInit(GetModuleHandleA(nullptr), kLaunchInfo);

    sm::Span<const char*> args{argv, size_t(argc)};
    LOG_INFO(GlobalLog, "args = [{}]", fmt::join(args, ", "));

    if (int err = sm::parseCommandLine(argc, argv)) {
        return (err == -1) ? 0 : err; // TODO: this is a little silly, should wrap in a type
    }

    int result = commonMain();

    LOG_INFO(ClientLog, "editor exiting with {}", result);

    return result;
} catch (const db::DbException& err) {
    LOG_ERROR(ClientLog, "database error: {}", err.error());
    return -1;
} catch (const std::exception& err) {
    LOG_ERROR(ClientLog, "unhandled exception: {}", err.what());
    return -1;
} catch (...) {
    LOG_ERROR(ClientLog, "unknown unhandled exception");
    return -1;
}

int WinMain(HINSTANCE hInstance, SM_UNUSED HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    auto _ = launch::commonInit(GetModuleHandleA(nullptr), kLaunchInfo);

    LOG_INFO(ClientLog, "lpCmdLine = {}", lpCmdLine);
    LOG_INFO(ClientLog, "nShowCmd = {}", nShowCmd);
    // TODO: parse lpCmdLine

    int result = commonMain();

    LOG_INFO(ClientLog, "editor exiting with {}", result);

    return result;
}
