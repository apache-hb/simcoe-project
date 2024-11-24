#include "stdafx.hpp"

#include "core/error/error.hpp"

#include <fmtlib/format.h>

using AnyError = sm::errors::AnyError;

AnyError::AnyError(std::string message, bool trace)
    : mMessage(std::move(message))
    , mStacktrace(trace ? std::stacktrace::current() : std::stacktrace{})
{ }

AnyError::AnyError()
    : AnyError("SUCCESS")
{ }
