#include "stdafx.hpp"

#include "config/option.hpp"

using namespace sm;
using namespace sm::config;

UpdateResult Context::updateFromConfigFile(std::istream& is) noexcept {
    auto parsed = toml::parse(is);
    if (parsed.failed()) {
        UpdateResult result;
        result.errors.emplace_back(UpdateError { UpdateStatus::eSyntaxError, std::string{parsed.error().description()} });
        return result;
    }

    auto root = parsed.table();

    return UpdateResult{};
}
