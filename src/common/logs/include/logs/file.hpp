#pragma once

#include "core/macros.hpp"
#include "io/impl/file.h"

#include "core/storage.hpp"

#include "logs/logs.hpp"

#include <expected>

namespace sm::logs {
    class FileChannel final : public ILogChannel {
        sm::Storage<io_t, IO_FILE_SIZE> mStream{};
        char mBuffer[2048];

        void acceptMessage(const Message& message) noexcept override;
        void closeChannel() noexcept override;
    public:
        FileChannel() noexcept = default;
        FileChannel(FileChannel&&) noexcept = default;
        FileChannel& operator=(FileChannel&&) noexcept = default;

        SM_NOCOPY(FileChannel)

        static std::expected<FileChannel, os_error_t> open(const char *path) noexcept;

        friend void swap(FileChannel& lhs, FileChannel& rhs) noexcept {
            std::swap(lhs.mStream, rhs.mStream);
            // dont swap the buffer contents, its just scratch space
        }
    };
}
