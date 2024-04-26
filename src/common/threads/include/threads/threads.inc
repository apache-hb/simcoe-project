#ifndef THREAD_CLASS
#   define THREAD_CLASS(id, name)
#endif

// this does *NOT* map to windows realtime priority class.
// stuff that really needs to be done quickly or the user will notice
// really just audio
THREAD_CLASS(eRealtime, "realtime")

// stuff that should be pretty responsive
// rendering, input, game thread, etc
THREAD_CLASS(eNormal, "normal")

// worker threads, stuff like shader compiling
// disk io, network io, etc
THREAD_CLASS(eWorker, "worker")

// very low priority, only used for background tasks
// stuff like polling once a second or less
THREAD_CLASS(eIdle, "idle")

#undef THREAD_CLASS

#ifndef PRIORITY_CLASS
#   define PRIORITY_CLASS(id, name, value)
#endif

PRIORITY_CLASS(eHigh, "high", HIGH_PRIORITY_CLASS)
PRIORITY_CLASS(eNormal, "normal", NORMAL_PRIORITY_CLASS)
PRIORITY_CLASS(eIdle, "idle", THREAD_PRIORITY_IDLE)

#undef PRIORITY_CLASS

#ifndef CACHE_TYPE
#   define CACHE_TYPE(id, name)
#endif

CACHE_TYPE(eUnified, "unified")
CACHE_TYPE(eInstruction, "instruction")
CACHE_TYPE(eData, "data")
CACHE_TYPE(eTrace, "trace")
CACHE_TYPE(eUnknown, "unknown")

#undef CACHE_TYPE
