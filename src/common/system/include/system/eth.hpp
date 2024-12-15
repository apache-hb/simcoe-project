#pragma once

#include "os/core.h"

#if CT_OS_WINDOWS
#   include "win32/eth.hpp"
#elif CT_OS_LINUX
#   include "posix/eth.hpp"
#endif
