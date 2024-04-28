#pragma once

#include "core/span.hpp"
#include "core/string.hpp"
#include "core/unique.hpp"

#include "fs/fs.h"

namespace sm {
    using FsHandle = sm::FnUniquePtr<fs_t, fs_delete>;

    class IFileSystem {
    public:
        virtual ~IFileSystem() = default;

        virtual sm::Span<const byte> readFileData(sm::StringView path) = 0;
    };
}
