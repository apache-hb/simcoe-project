#include "stdafx.hpp"

#include "common.hpp"

#include "core/error.hpp"
#include "logs/file.hpp"

using namespace sm;
using namespace sm::logs;

void FileChannel::acceptMessage(const Message &message) noexcept {
    size_t length = logs::buildMessageHeader(mBuffer, message);
    char *start = mBuffer + length;
    size_t remaining = sizeof(mBuffer) - length;

    for (sm::StringView line : logs::splitMessage(message.message)) {
        auto [_, extra] = fmt::format_to_n(start, remaining, " {}\n", line);
        io_write(mStream.data(), mBuffer, length + extra);
    }
}

void FileChannel::closeChannel() noexcept {
    io_close(mStream.data());
}

std::expected<FileChannel, os_error_t> FileChannel::open(const char *path) noexcept {
    FileChannel channel;
    io_t *io = io_file_init(channel.mStream.ptr(), path, eOsAccessWrite | eOsAccessTruncate);
    os_error_t err = io_error(io);
    if (err != eOsSuccess) {
        return std::unexpected(err);
    }

    return channel;
}
