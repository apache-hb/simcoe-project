#pragma once

namespace sm::threads {
    bool isRealtimeThread() noexcept;
    void setCurrentThreadClass(bool realtime) noexcept;
}

#define SM_BLOCKING_ZONE(name) \
    CTASSERTF(!sm::threads::isRealtimeThread(), "Blocking zone " #name " called from realtime thread");
