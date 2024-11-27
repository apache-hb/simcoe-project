#pragma once

#if _WIN32
#   include "core/win32.hpp"
#endif

#include "core/fs.hpp"

#include "db/db.hpp"

#ifndef _WIN32
typedef void *HINSTANCE;
#endif

namespace sm::launch {
    struct LaunchResult {
        ~LaunchResult() noexcept;

        bool shouldExit() const noexcept;
        int exitCode() const noexcept;
    };

    struct LaunchInfo {
        db::DbType logDbType;
        db::ConnectionConfig logDbConfig;

        fs::path logPath;

        db::DbType infoDbType;
        db::ConnectionConfig infoDbConfig;

        bool threads = true;
        bool network = false;
        bool com = false;
        bool glfw = false;

        bool allowInvalidArgs = false;
    };

    LaunchResult commonInit(HINSTANCE hInstance, const LaunchInfo& info);
    LaunchResult commonInitMain(int argc, const char **argv, const LaunchInfo& info) noexcept;
    LaunchResult commonInitWinMain(HINSTANCE hInstance, int nShowCmd, const LaunchInfo& info) noexcept;
}

#if _WIN32
#   define SM_LAUNCH_WINMAIN_BODY(NAME, ENTRY, INFO) \
        int WinMain(HINSTANCE hInstance, SM_UNUSED HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) try { \
                launch::LaunchResult launch = launch::commonInitWinMain(hInstance, nShowCmd, INFO); \
                if (launch.shouldExit()) { \
                    return launch.exitCode(); \
                } \
                int result = ENTRY(); \
                LOG_INFO(GlobalLog, NAME " exiting with {}", result); \
                return result; \
            } catch (const std::exception& err) { \
                LOG_ERROR(GlobalLog, "unhandled exception: {}", err.what()); \
                return -1; \
            } catch (...) { \
                LOG_ERROR(GlobalLog, "unknown unhandled exception"); \
                return -1; \
            }
#else
#   define SM_LAUNCH_WINMAIN_BODY(NAME, ENTRY, INFO)
#endif

#define SM_LAUNCH_MAIN_BODY(NAME, ENTRY, INFO) \
    int main(int argc, const char **argv) noexcept try { \
        sm::launch::LaunchResult launch = sm::launch::commonInitMain(argc, argv, INFO); \
        if (launch.shouldExit()) { \
            return launch.exitCode(); \
        } \
        int result = ENTRY(); \
        LOG_INFO(GlobalLog, NAME " exiting with {}", result); \
        return result; \
    } catch (const std::exception& err) { \
        LOG_ERROR(GlobalLog, "unhandled exception: {}", err.what()); \
        return -1; \
    } catch (...) { \
        LOG_ERROR(GlobalLog, "unknown unhandled exception"); \
        return -1; \
    }

#define SM_LAUNCH_MAIN(NAME, ENTRY, INFO) \
    SM_LAUNCH_MAIN_BODY(NAME, ENTRY, INFO) \
    SM_LAUNCH_WINMAIN_BODY(NAME, ENTRY, INFO)
