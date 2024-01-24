#pragma once

#include "fmtlib/format.h"

#include "logs.reflect.h"

namespace sm::logs {
    class ILogger {
        Severity m_severity;

    protected:
        constexpr ILogger(Severity severity)
            : m_severity(severity)
        { }

        virtual void accept(const Message& message) = 0;

    public:
        virtual ~ILogger() = default;

        void log(Category category, Severity severity, std::string_view msg);

        constexpr Severity get_severity() const { return m_severity; }
        constexpr bool will_accept(Severity severity) const { return severity >= m_severity; }
    };

    template<Category::Inner C> requires (Category{C}.is_valid())
    class Sink final {
        ILogger& m_logger;

    public:
        constexpr Sink(ILogger& logger)
            : m_logger(logger)
        { }

        void operator()(Severity severity, std::string_view msg, auto&&... args) const {
            log(severity, msg, std::forward<decltype(args)>(args)...);
        }

        void log(Severity severity, std::string_view msg, auto&&... args) const {
            m_logger.log(C, severity, fmt::vformat(msg, fmt::make_format_args(args...)));
        }

        void trace(std::string_view msg, auto&&... args) const {
            log(Severity::eTrace, msg, std::forward<decltype(args)>(args)...);
        }

        void debug(std::string_view msg, auto&&... args) const {
            log(Severity::eInfo, msg, std::forward<decltype(args)>(args)...);
        }

        void info(std::string_view msg, auto&&... args) const {
            log(Severity::eInfo, msg, std::forward<decltype(args)>(args)...);
        }

        void warn(std::string_view msg, auto&&... args) const {
            log(Severity::eWarning, msg, std::forward<decltype(args)>(args)...);
        }

        void error(std::string_view msg, auto&&... args) const {
            log(Severity::eError, msg, std::forward<decltype(args)>(args)...);
        }

        void fatal(std::string_view msg, auto&&... args) const {
            log(Severity::eFatal, msg, std::forward<decltype(args)>(args)...);
        }

        void panic(std::string_view msg, auto&&... args) const {
            log(Severity::ePanic, msg, std::forward<decltype(args)>(args)...);
        }
    };
}
