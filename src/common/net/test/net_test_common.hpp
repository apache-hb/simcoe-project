#pragma once

#include "test/common.hpp"

#include <mutex>
#include <vector>
#include <string>

#include <fmtlib/format.h>

struct NetTestStream {
    std::mutex errorMutex;
    std::vector<std::string> errors;

    void add(std::string_view fmt, auto&&... args) {
        std::lock_guard guard(errorMutex);
        errors.push_back(fmt::vformat(fmt, fmt::make_format_args(args...)));
    }

    void expect(bool condition, std::string_view fmt, auto&&... args) {
        if (!condition)
            add(fmt, std::forward<decltype(args)>(args)...);
    }

    ~NetTestStream() {
        CHECK(errors.empty());
        for (const auto& error : errors) {
            FAIL(error);
        }
    }
};
