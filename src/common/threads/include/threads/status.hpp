#pragma once

namespace sm::threads {
    bool isRealtimeThread();
    void setCurrentThreadClass(bool realtime);
}
