#include "stdafx.hpp"

#include "core/error/error.hpp"

#include <fmtlib/format.h>

namespace errors = sm::errors;

errors::AnyError::AnyError(std::string message, bool trace) noexcept
    : mMessage(std::move(message))
    , mStacktrace(trace ? std::stacktrace::current() : std::stacktrace{})
{ }
