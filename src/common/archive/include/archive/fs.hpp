#pragma once

#include "base/fs.hpp"
#include "core/span.hpp"

namespace sm {
    class IFileSystem {
    public:
        virtual ~IFileSystem() noexcept = default;

        virtual sm::View<byte> readFileData(const fs::path& path) = 0;
    };

    IFileSystem *mountFileSystem(fs::path path);
    IFileSystem *mountArchive(const fs::path& path);
}
