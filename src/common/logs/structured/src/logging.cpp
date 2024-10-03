#include "stdafx.hpp"

#include "logs/structured/logging.hpp"
#include "logs/structured/logger.hpp"

namespace structured = sm::logs::structured;

using CategoryInfo = sm::logs::structured::CategoryInfo;
using MessageInfo = sm::logs::structured::MessageInfo;

using CategoryId = sm::logs::structured::detail::CategoryId;
using MessageId = sm::logs::structured::detail::MessageId;

static_assert(BUILD_MESSAGE_ATTRIBUTES_IMPL("just text").size() == 0);
static_assert(BUILD_MESSAGE_ATTRIBUTES_IMPL("thing {0}", 1).size() == 1);
static_assert(BUILD_MESSAGE_ATTRIBUTES_IMPL("multiple {0} second {0} identical {0} indices", 1).size() == 1);
static_assert(BUILD_MESSAGE_ATTRIBUTES_IMPL("thing {0} second {1} third {name}", 1, 2, fmt::arg("name", "bob")).size() == 3);

static std::vector<MessageInfo> &getLogMessages() noexcept {
    static std::vector<MessageInfo> sLogMessages;
    return sLogMessages;
}

static std::vector<CategoryInfo> &getLogCategories() noexcept {
    static std::vector<CategoryInfo> sLogCategories;
    return sLogCategories;
}

MessageId::MessageId(const MessageInfo& info) noexcept
    : data(info)
{
    getLogMessages().emplace_back(data);
}

CategoryId::CategoryId(CategoryInfo info) noexcept
    : data(info)
{
    getLogCategories().emplace_back(data);
}

structured::MessageStore structured::getMessages() noexcept {
    structured::MessageStore result {
        .categories = getLogCategories(),
        .messages = getLogMessages(),
    };

    return result;
}

void structured::detail::postLogMessage(const MessageInfo& message, ArgStore args) noexcept {
    Logger::instance().postMessage(message, std::move(args));
}
