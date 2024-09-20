#pragma once

#include <simcoe_config.h>
#include <fmtlib/format.h> // IWYU pragma: export

#include "core/core.hpp"
#include "core/string.hpp"
#include "core/adt/vector.hpp"

#include "logs.meta.hpp"

typedef struct colour_pallete_t colour_pallete_t;

#define LOG_CATEGORY(id) extern sm::logs::LogCategory id
#define LOG_CATEGORY_IMPL(id, name) sm::logs::LogCategory id(name)

namespace sm::logs {
    REFLECT_ENUM(Severity)
    enum class Severity {
        eTrace = 0,
        eDebug = 1,
        eInfo = 2,
        eWarning = 3,
        eError = 4,
        eFatal = 5,
        ePanic = 6,

        eCount
    };

    class LogCategory final {
        sm::String mName;

        void logString(Severity severity, sm::StringView msg) const noexcept;
        void vlog(Severity severity, fmt::string_view format, fmt::format_args args) const noexcept;

    public:
        constexpr LogCategory(sm::StringView name) noexcept
            : mName(name)
        { }

        constexpr sm::StringView name() const noexcept {
            return mName;
        }

        template<typename... A>
        void log(Severity severity, fmt::format_string<A...> msg, A&&... args) const noexcept {
            vlog(severity, msg, fmt::make_format_args(args...));
        }

        template<typename... A>
        void operator()(Severity severity, fmt::format_string<A...> msg, A&&... args) const noexcept {
            vlog(severity, msg, fmt::make_format_args(args...));
        }

        template<typename... A>
        void trace(fmt::format_string<A...> msg, A&&... args) const noexcept {
            vlog(Severity::eTrace, msg, fmt::make_format_args(args...));
        }

        template<typename... A>
        void info(fmt::format_string<A...> msg, A&&... args) const noexcept {
            vlog(Severity::eInfo, msg, fmt::make_format_args(args...));
        }

        template<typename... A>
        void warn(fmt::format_string<A...> msg, A&&... args) const noexcept {
            vlog(Severity::eWarning, msg, fmt::make_format_args(args...));
        }

        template<typename... A>
        void error(fmt::format_string<A...> msg, A&&... args) const noexcept {
            vlog(Severity::eError, msg, fmt::make_format_args(args...));
        }

        template<typename... A>
        void fatal(fmt::format_string<A...> msg, A&&... args) const noexcept {
            vlog(Severity::eFatal, msg, fmt::make_format_args(args...));
        }

        template<typename... A>
        void panic(fmt::format_string<A...> msg, A&&... args) const noexcept {
            vlog(Severity::ePanic, msg, fmt::make_format_args(args...));
        }

        void traceString(sm::StringView msg) const noexcept { logString(Severity::eTrace, msg); }
        void infoString(sm::StringView msg) const noexcept { logString(Severity::eInfo, msg); }
        void warnString(sm::StringView msg) const noexcept { logString(Severity::eWarning, msg); }
        void errorString(sm::StringView msg) const noexcept { logString(Severity::eError, msg); }
        void fatalString(sm::StringView msg) const noexcept { logString(Severity::eFatal, msg); }
        void panicString(sm::StringView msg) const noexcept { logString(Severity::ePanic, msg); }

        constexpr auto operator<=>(const LogCategory&) const = default;
    };

    struct Message final {
        sm::StringView message;
        const LogCategory& category;
        Severity severity;
        uint32 timestamp;
        uint16 thread;
    };

    class ILogChannel {
    public:
        virtual ~ILogChannel() = default;

        virtual void acceptMessage(const Message &message) noexcept = 0;
        virtual void closeChannel() noexcept = 0;
    };

    class Logger final {
        Severity mSeverity;
        sm::Vector<ILogChannel*> mChannels;

        void log(const Message &message) noexcept;

    public:
        constexpr Logger(Severity severity) noexcept
            : mSeverity(severity)
        { }

        void log(const LogCategory& category, Severity severity, sm::StringView msg) noexcept;

        void addChannel(ILogChannel& channel) noexcept;
        void removeChannel(ILogChannel& channel) noexcept;

        void shutdown() noexcept;

        constexpr void setSeverity(Severity severity) noexcept {
            mSeverity = severity;
        }

        constexpr Severity getSeverity() const noexcept {
            return mSeverity;
        }

        constexpr bool willAcceptMessage(Severity severity) const noexcept {
            return severity >= mSeverity;
        }
    };

    LOG_CATEGORY(gGlobal);
    LOG_CATEGORY(gDebug);
    LOG_CATEGORY(gAssets);

    Logger& getGlobalLogger() noexcept;
    void shutdown() noexcept;

    bool isDebugConsoleAvailable() noexcept;
    ILogChannel& getDebugConsole() noexcept;

    bool isConsoleHandleAvailable() noexcept;
    ILogChannel& getConsoleHandle() noexcept;
} // namespace sm::logs
