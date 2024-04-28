#pragma once

#include "core/fs.hpp"
#include "core/span.hpp"

namespace sm {
    class IFileSystem {
    public:
        virtual ~IFileSystem() = default;

        virtual sm::View<byte> readFileData(const fs::path& path) = 0;
    };

    IFileSystem *mountFileSystem(fs::path path);
    IFileSystem *mountArchive(const fs::path& path);
}
