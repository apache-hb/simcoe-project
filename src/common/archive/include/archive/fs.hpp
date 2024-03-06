#pragma once

#include "core/unique.hpp"

#include "fs/fs.h"

namespace sm {
    using FsHandle = sm::FnUniquePtr<fs_t, fs_delete>;
}
