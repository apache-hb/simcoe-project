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

        void accept(const Message& message) noexcept override;
    public:
        FileChannel() noexcept = default;
        ~FileChannel() noexcept;

        SM_NOCOPY(FileChannel)

        FileChannel(FileChannel&& other) noexcept;
        FileChannel& operator=(FileChannel&& other) noexcept;

        static std::expected<FileChannel, io_error_t> open(const char *path) noexcept;

        friend void swap(FileChannel& lhs, FileChannel& rhs) noexcept {
            std::swap(lhs.mStream, rhs.mStream);
            // dont swap the buffer contents, its just scratch space
        }
    };
}
