#include "stdafx.hpp"

#include "common.hpp"

#include "logs/file.hpp"

using namespace sm;
using namespace sm::logs;

FileChannel::~FileChannel() noexcept {
    // TODO: io->cb is private
    if (io_t *io = mStream.data(); io->cb != nullptr) {
        io_close(io);
        io->cb = nullptr;
    }
}

FileChannel::FileChannel(FileChannel&& other) noexcept {
    swap(*this, other);
}

FileChannel& FileChannel::operator=(FileChannel&& other) noexcept {
    swap(*this, other);
    return *this;
}

void FileChannel::accept(const Message &message) noexcept {
    size_t length = logs::buildMessageHeader(mBuffer, message);
    char *start = mBuffer + length;
    size_t remaining = sizeof(mBuffer) - length;

    for (sm::StringView line : logs::splitMessage(message.message)) {
        auto [_, extra] = fmt::format_to_n(start, remaining, " {}\n", line);
        io_write(mStream.data(), mBuffer, length + extra);
    }
}

std::expected<FileChannel, io_error_t> FileChannel::open(const char *path) noexcept {
    FileChannel channel;
    io_t *io = io_file_init(channel.mStream.ptr(), path, eOsAccessWrite | eOsAccessTruncate);
    io_error_t err = io_error(io);
    if (err != eOsSuccess) {
        return std::unexpected(err);
    }

    return channel;
}
