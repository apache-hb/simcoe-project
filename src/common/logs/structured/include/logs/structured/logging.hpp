#pragma once

#include <simcoe_config.h>

#include "logs/structured/category.hpp"
#include "logs/structured/message.hpp"

namespace sm::logs::structured {
    struct MessageStore {
        std::span<const CategoryInfo> categories;
        std::span<const MessageInfo> messages;
    };

    MessageStore getMessages() noexcept;

    namespace detail {
        void postLogMessage(const MessageInfo& message, std::unique_ptr<DynamicArgStore> args) noexcept;

        template<LogMessageFn F, typename... A>
        void fmtMessage(A&&... args) noexcept {
            const MessageId& message = gTagInfo<F, A...>;
            const MessageInfo& info = message.data;

            using Data = ArgStoreData<A...>;
            std::unique_ptr<Data> data = std::make_unique<Data>(std::forward<A>(args)...);
            postLogMessage(info, std::move(data));

#if 0
            ArgStore store;
            store.reserve(info.attributeCount(), info.namedAttributes.size());
            ((store.push_back(std::forward<A>(args))), ...);

            postLogMessage(info, std::move(store));
#endif
        }
    }
} // namespace sm::logs

#define LOG_MESSAGE_CATEGORY(id, name) \
    struct id final : public sm::logs::structured::CategoryInfo { \
        constexpr id() noexcept \
            : CategoryInfo(sm::logs::structured::CategoryInfo{name}) \
        { } \
    }

#define BUILD_MESSAGE_DATA_IMPL(message, severity, category, location, info) \
    sm::logs::structured::MessageInfo { message, severity, sm::logs::structured::detail::gLogCategory<category>.data, location, (info).indices, (info).namedAttributes() }

#define LOG_MESSAGE(category, severity, message, ...) \
    do { \
        static constexpr std::source_location kSourceLocation = std::source_location::current(); \
        static constexpr auto kMessageInfo = BUILD_MESSAGE_ATTRIBUTES_IMPL(message, __VA_ARGS__); \
        struct LogMessageImpl { \
            constexpr auto operator()() const noexcept { \
                return BUILD_MESSAGE_DATA_IMPL(message, severity, category, kSourceLocation, kMessageInfo); \
            } \
        }; \
        sm::logs::structured::detail::fmtMessage<LogMessageImpl>(__VA_ARGS__); \
    } while (false)

#define LOG_TRACE(category, ...) LOG_MESSAGE(category, sm::logs::Severity::eTrace,   __VA_ARGS__)
#define LOG_DEBUG(category, ...) LOG_MESSAGE(category, sm::logs::Severity::eDebug,   __VA_ARGS__)
#define LOG_INFO(category, ...)  LOG_MESSAGE(category, sm::logs::Severity::eInfo,    __VA_ARGS__)
#define LOG_WARN(category, ...)  LOG_MESSAGE(category, sm::logs::Severity::eWarning, __VA_ARGS__)
#define LOG_ERROR(category, ...) LOG_MESSAGE(category, sm::logs::Severity::eError,   __VA_ARGS__)
#define LOG_FATAL(category, ...) LOG_MESSAGE(category, sm::logs::Severity::eFatal,   __VA_ARGS__)
#define LOG_PANIC(category, ...) LOG_MESSAGE(category, sm::logs::Severity::ePanic,   __VA_ARGS__)

LOG_MESSAGE_CATEGORY(GlobalLog, "Global");
