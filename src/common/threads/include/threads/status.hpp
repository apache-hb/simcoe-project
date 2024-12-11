#pragma once

namespace sm::threads {
    bool isRealtimeThread() noexcept [[clang::nonblocking]];
    void setCurrentThreadClass(bool realtime) noexcept [[clang::nonblocking]];
}

#define SM_BLOCKING_ZONE(name) \
    CTASSERTF(!sm::threads::isRealtimeThread(), "Blocking zone " #name " called from realtime thread");
