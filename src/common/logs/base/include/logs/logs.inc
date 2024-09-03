#ifndef LOG_SEVERITY
#   define LOG_SEVERITY(id, name, level)
#endif

LOG_SEVERITY(eTrace,   "trace", 0)
LOG_SEVERITY(eDebug,   "debug", 1)
LOG_SEVERITY(eInfo,    "info",  2)
LOG_SEVERITY(eWarning, "warn",  3)
LOG_SEVERITY(eError,   "error", 4)
LOG_SEVERITY(eFatal,   "fatal", 5)
LOG_SEVERITY(ePanic,   "panic", 6)

#undef LOG_SEVERITY
