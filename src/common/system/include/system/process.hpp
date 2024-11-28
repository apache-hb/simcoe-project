#pragma once

#include "os/core.h"

#if CT_OS_WINDOWS
#   include "win32/process.hpp"
#elif CT_OS_LINUX
#   include "posix/process.hpp"
#endif
