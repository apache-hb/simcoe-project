#include "threads/status.hpp"

static thread_local bool gIsRealtimeThread = false;

bool sm::threads::isRealtimeThread() noexcept {
    return gIsRealtimeThread;
}

void sm::threads::setCurrentThreadClass(bool realtime) noexcept {
    gIsRealtimeThread = realtime;
}
