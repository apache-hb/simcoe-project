#include "stdafx.hpp"

#include "core/error/error.hpp"

#include <fmtlib/format.h>

using namespace sm;

AnyError::AnyError(int code, std::string message, bool trace) noexcept
    : mCode(code)
    , mMessage(std::move(message))
    , mStacktrace(trace ? std::stacktrace::current() : std::stacktrace{})
{ }

AnyError::AnyError(int code, std::string_view message, bool trace)
    : Self(code, fmt::format("{} ({})", message, code), trace)
{ }
