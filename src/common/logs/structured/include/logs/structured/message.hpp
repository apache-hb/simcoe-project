#pragma once

#include <simcoe_config.h>

#include <string_view>
#include <source_location>
#include <variant>

#include "logs/logs.hpp"
#include "orm/error.hpp"

#include <fmtlib/format.h>
#include <fmt/compile.h>

namespace sm::db {
    class Connection;
}

namespace sm::logs::structured {
    struct MessageAttributeInfo {
        std::string_view name;
    };

    using MessageAttribute = std::variant<
        std::string,
        std::int64_t,
        float,
        bool
    >;

    struct LogMessageInfo {
        logs::Severity level;
        std::string_view message;
        std::source_location location;
    };

    db::DbError setup(db::Connection& connection) noexcept;

    namespace detail {
        template<typename T>
        concept LogMessageFn = requires(T fn) {
            { fn() } -> std::same_as<LogMessageInfo>;
        };

        struct LogMessageId {
            LogMessageId(const LogMessageInfo& message) noexcept;
            const std::uint64_t hash;
            const std::vector<MessageAttributeInfo> attributes;
        };

        template<LogMessageFn F, typename... A>
        const LogMessageInfo& getMessage() noexcept {
            F fn{};
            static LogMessageInfo message{fn()};
            return message;
        }

        template<LogMessageFn F, typename... A>
        inline LogMessageId gTagInfo{&getMessage<F, A...>()};

        void postLogMessage(const LogMessageInfo& message, fmt::format_args args) noexcept;

        template<LogMessageFn F, typename... A>
        void fmtMessage(fmt::format_string<A...> fmt, A&&... args) noexcept {
            const auto& message = gTagInfo<F, A...>;
            postLogMessage(message, fmt::make_format_args(args...));
        }
    }
} // namespace sm::logs
