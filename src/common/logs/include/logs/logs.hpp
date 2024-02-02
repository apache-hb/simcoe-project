#pragma once

#include "core/format.hpp"

#include "logs.reflect.h"

#include <iterator>

namespace sm { class IArena; }

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
        void log(const Message& message);

        constexpr Severity get_severity() const { return m_severity; }
        constexpr bool will_accept(Severity severity) const { return severity >= m_severity; }
    };

    /// @brief a logging sink that wraps a logger and the category of the calling code
    /// @tparam C the category of the calling code
    /// @warning this class is not thread-safe
    template<Category::Inner C> requires (Category{C}.is_valid())
    class Sink final {
        mutable FormatPoolBuffer<Pool::eLogging> m_buffer;

        ILogger& m_logger;

    public:
        constexpr Sink(ILogger& logger)
            : m_logger(logger)
        { }

        constexpr Sink(const Sink& other)
            : m_logger(other.m_logger)
        { }

        void operator()(Severity severity, std::string_view msg, auto&&... args) const {
            log(severity, msg, std::forward<decltype(args)>(args)...);
        }

        void log(Severity severity, std::string_view msg, auto&&... args) const {
            // while ILogger will reject the message, still do an early return
            // to avoid formatting if we don't need to
            if (!m_logger.will_accept(severity)) { return; }

            auto out = std::back_inserter(m_buffer);
            fmt::vformat_to(out, msg, fmt::make_format_args(args...));

            m_logger.log(C, severity, std::string_view { m_buffer.data(), m_buffer.size() });

            m_buffer.clear();
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
