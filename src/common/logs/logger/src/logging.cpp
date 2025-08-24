#include "stdafx.hpp"

#include "logger/logging.hpp"
#include "logger/logger.hpp"

namespace logs = sm::logs;

using CategoryInfo = sm::logs::CategoryInfo;
using MessageInfo = sm::logs::MessageInfo;

using CategoryId = sm::logs::detail::CategoryId;
using MessageId = sm::logs::detail::MessageId;

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

logs::MessageStore logs::getMessages() noexcept {
    logs::MessageStore result {
        .categories = getLogCategories(),
        .messages = getLogMessages(),
    };

    return result;
}

void logs::detail::postLogMessage(const MessageInfo& message, std::unique_ptr<DynamicArgStore> args) noexcept {
    Logger::instance().postMessage(message, std::move(args));
}
