#pragma once

#include <simcoe_config.h>

#include "core/core.hpp"
#include "core/string.hpp"
#include "core/vector.hpp"

#include "fmtlib/format.h" // IWYU pragma: export

#include "logs.reflect.h"

#define LOG_CATEGORY(id) \
    extern sm::logs::LogCategory id;

#define LOG_CATEGORY_IMPL(id, name) \
    sm::logs::LogCategory id(name);

namespace sm::logs {
    class LogCategory {
        sm::String mName;

        void vlog(Severity severity, fmt::string_view format, fmt::format_args args) const;

    public:
        constexpr LogCategory(sm::StringView name)
            : mName(name)
        { }

        constexpr sm::StringView name() const {
            return mName;
        }

        template<typename... A>
        void log(Severity severity, fmt::format_string<A...> msg, A&&... args) const {
            vlog(severity, msg, fmt::make_format_args(args...));
        }

        template<typename... A>
        void operator()(Severity severity, fmt::format_string<A...> msg, A&&... args) const {
            vlog(severity, msg, fmt::make_format_args(args...));
        }

        template<typename... A>
        void trace(fmt::format_string<A...> msg, A&&... args) const {
            vlog(Severity::eTrace, msg, fmt::make_format_args(args...));
        }

        template<typename... A>
        void info(fmt::format_string<A...> msg, A&&... args) const {
            vlog(Severity::eInfo, msg, fmt::make_format_args(args...));
        }

        template<typename... A>
        void warn(fmt::format_string<A...> msg, A&&... args) const {
            vlog(Severity::eWarning, msg, fmt::make_format_args(args...));
        }

        template<typename... A>
        void error(fmt::format_string<A...> msg, A&&... args) const {
            vlog(Severity::eError, msg, fmt::make_format_args(args...));
        }

        template<typename... A>
        void fatal(fmt::format_string<A...> msg, A&&... args) const {
            vlog(Severity::eFatal, msg, fmt::make_format_args(args...));
        }

        template<typename... A>
        void panic(fmt::format_string<A...> msg, A&&... args) const {
            vlog(Severity::ePanic, msg, fmt::make_format_args(args...));
        }

        constexpr auto operator<=>(const LogCategory&) const = default;
    };

    struct Message {
        sm::StringView message;
        const LogCategory& category;
        Severity severity;
        uint32 timestamp;
        uint16 thread;
    };

    class ILogChannel {
    public:
        virtual ~ILogChannel() = default;

        virtual void accept(const Message &message) = 0;
    };

    class Logger {
        Severity mSeverity;
        sm::Vector<ILogChannel*> mChannels;

        void log(const Message &message);

    public:
        constexpr Logger(Severity severity)
            : mSeverity(severity)
        { }

        virtual ~Logger() = default;

        void log(Category category, Severity severity, sm::StringView msg);
        void log(const LogCategory& category, Severity severity, sm::StringView msg);

        void add_channel(ILogChannel *channel);
        void remove_channel(ILogChannel *channel);

        void set_severity(Severity severity);
        Severity get_severity() const;

        constexpr bool will_accept(Severity severity) const {
            return severity >= mSeverity;
        }
    };

    LOG_CATEGORY(gGlobal);
    LOG_CATEGORY(gRender);
    LOG_CATEGORY(gDebug);
    LOG_CATEGORY(gGpuApi);
    LOG_CATEGORY(gAssets);

    Logger& get_logger() noexcept;
} // namespace sm::logs
