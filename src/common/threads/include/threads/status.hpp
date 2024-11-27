#pragma once

namespace sm::threads {
    bool isRealtimeThread();
    void setCurrentThreadClass(bool realtime);
}

#define SM_BLOCKING_ZONE(name) \
    CTASSERTF(!sm::threads::isRealtimeThread(), "Blocking zone " #name " called from realtime thread");
