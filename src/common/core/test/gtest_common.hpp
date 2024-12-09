#pragma once

#include <gtest/gtest.h>

#include <fmtlib/format.h>

#include "core/string.hpp"

#include "base/panic.h"
#include "core/macros.h"
#include "os/core.h"

class ThisEnvironment : public testing::Environment {
    void SetUp() override {
        gPanicHandler = [](source_info_t info, const char *msg, va_list args) {
            auto err = sm::vformat(msg, args);

            fmt::println(stderr, "ASSERT : {}", info.function);
            fmt::println(stderr, "WHERE  : {}:{}", info.file, info.line);
            fmt::println(stderr, "--------");
            fmt::println(stderr, "{}", err);

            os_exit(CT_EXIT_INTERNAL);
        };
    }

    static testing::Environment *gInstance;
};

testing::Environment *ThisEnvironment::gInstance = testing::AddGlobalTestEnvironment(new ThisEnvironment);
