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

    public:
        constexpr Sink(Logger& logger, Category category)
            : mLogger(logger)
            , mCategory(category)
        { }

        void log(Severity severity, std::string_view msg, auto &&...args) const {
            // while ILogger will reject the message, still do an early return
            // to avoid formatting if we don't need to
            if (!mLogger.will_accept(severity)) return;

            auto text = fmt::vformat(msg, fmt::make_format_args(args...));
            mLogger.log(mCategory, severity, text);
        }

        void operator()(Severity severity, std::string_view msg, auto &&...args) const {
            log(severity, msg, std::forward<decltype(args)>(args)...);
        }

        void trace(std::string_view msg, auto &&...args) const {
            log(Severity::eTrace, msg, std::forward<decltype(args)>(args)...);
        }

        void debug(std::string_view msg, auto &&...args) const {
            log(Severity::eInfo, msg, std::forward<decltype(args)>(args)...);
        }

        void info(std::string_view msg, auto &&...args) const {
            log(Severity::eInfo, msg, std::forward<decltype(args)>(args)...);
        }

        void warn(std::string_view msg, auto &&...args) const {
            log(Severity::eWarning, msg, std::forward<decltype(args)>(args)...);
        }

        void error(std::string_view msg, auto &&...args) const {
            log(Severity::eError, msg, std::forward<decltype(args)>(args)...);
        }

        void fatal(std::string_view msg, auto &&...args) const {
            log(Severity::eFatal, msg, std::forward<decltype(args)>(args)...);
        }

        void panic(std::string_view msg, auto &&...args) const {
            log(Severity::ePanic, msg, std::forward<decltype(args)>(args)...);
        }
    };

    Logger& get_logger() noexcept;
    Sink get_sink(Category category) noexcept;
} // namespace sm::logs
