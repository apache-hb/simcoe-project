#include "threads/status.hpp"

static thread_local bool gIsRealtimeThread = false;

bool sm::threads::isRealtimeThread() {
    return gIsRealtimeThread;
}

void sm::threads::setCurrentThreadClass(bool realtime) {
    gIsRealtimeThread = realtime;
}
