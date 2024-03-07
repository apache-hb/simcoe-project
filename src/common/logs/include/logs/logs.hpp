#pragma once

#include <simcoe_config.h>

#include "core/vector.hpp"

#include "fmtlib/format.h"

#include "logs.reflect.h"

namespace sm::logs {
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
        constexpr Logger(Severity severity) noexcept
            : mSeverity(severity)
        { }

        virtual ~Logger() = default;

        void log(Category category, Severity severity, std::string_view msg);

        void add_channel(ILogChannel *channel);
        void remove_channel(ILogChannel *channel);

        void set_severity(Severity severity);
        Severity get_severity() const;

        constexpr bool will_accept(Severity severity) const {
            return severity >= mSeverity;
        }
    };

    class Sink final {
        Logger& mLogger;
        Category mCategory;

        void vlog(Severity severity, fmt::string_view format, fmt::format_args args) const {
            if (!mLogger.will_accept(severity)) return;

            auto text = fmt::vformat(format, args);
            mLogger.log(mCategory, severity, text);
        }

    public:
        constexpr Sink(Logger& logger, Category category)
            : mLogger(logger)
            , mCategory(category)
        { }

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
    };

    Logger& get_logger() noexcept;
    Sink get_sink(Category category) noexcept;
} // namespace sm::logs
