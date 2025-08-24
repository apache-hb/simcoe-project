#pragma once

#include <string>
#include <stdint.h>

namespace sm::db {
    class TableSpace {
        std::string mName;
        std::string mPath;
        uint64_t mTotalSize;
        uint64_t mFreeSize;

    public:
        TableSpace(std::string name, std::string path, uint64_t totalSize, uint64_t freeSize) noexcept
            : mName(std::move(name))
            , mPath(std::move(path))
            , mTotalSize(totalSize)
            , mFreeSize(freeSize)
        { }

        [[nodiscard]] const std::string& getName() const noexcept { return mName; }
        [[nodiscard]] const std::string& getPath() const noexcept { return mPath; }
        [[nodiscard]] uint64_t getTotalSize() const noexcept { return mTotalSize; }
        [[nodiscard]] uint64_t getFreeSize() const noexcept { return mFreeSize; }
    };
}
