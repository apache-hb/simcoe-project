#include "stdafx.hpp"

#include "archive/io.hpp"
#include "archive/archive.hpp"

#include "core/memory.h"

using namespace sm;

WINDOWPLACEMENT sm::archive::toWindowPlacement(const sm::dao::archive::WindowPlacement &placement) noexcept {
    return WINDOWPLACEMENT {
        .length = sizeof(WINDOWPLACEMENT),
        .flags = placement.flags,
        .showCmd = placement.showCmd,
        .ptMinPosition = {
            .x = (LONG)placement.minPositionX,
            .y = (LONG)placement.minPositionY,
        },
        .ptMaxPosition = {
            .x = (LONG)placement.maxPositionX,
            .y = (LONG)placement.maxPositionY,
        },
        .rcNormalPosition = {
            .left = (LONG)placement.normalPosLeft,
            .top = (LONG)placement.normalPosTop,
            .right = (LONG)placement.normalPosRight,
            .bottom = (LONG)placement.normalPosBottom,
        },
    };
}

sm::dao::archive::WindowPlacement sm::archive::fromWindowPlacement(const WINDOWPLACEMENT &placement) noexcept {
    return sm::dao::archive::WindowPlacement {
        .flags = placement.flags,
        .showCmd = placement.showCmd,
        .minPositionX = placement.ptMinPosition.x,
        .minPositionY = placement.ptMinPosition.y,
        .maxPositionX = placement.ptMaxPosition.x,
        .maxPositionY = placement.ptMaxPosition.y,
        .normalPosLeft = placement.rcNormalPosition.left,
        .normalPosTop = placement.rcNormalPosition.top,
        .normalPosRight = placement.rcNormalPosition.right,
        .normalPosBottom = placement.rcNormalPosition.bottom,
    };
}

Io Io::file(const char *path, os_access_t access) {
    return Io(io_file(path, access, get_default_arena()));
}

OsError Io::error() const {
    return io_error(get());
}

const char *Io::name() const {
    return io_name(get());
}

size_t Io::size() const {
    return io_size(get());
}

size_t Io::read_bytes(void *data, size_t size) {
    return io_read(get(), data, size);
}

size_t Io::write_bytes(const void *data, size_t size) {
    return io_write(get(), data, size);
}
