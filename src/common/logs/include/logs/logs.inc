#ifndef LOG_SEVERITY
#   define LOG_SEVERITY(id, name, level)
#endif

LOG_SEVERITY(eTrace, "trace", 0)
LOG_SEVERITY(eInfo, "info", 1)
LOG_SEVERITY(eWarning, "warn", 2)
LOG_SEVERITY(eError, "error", 3)
LOG_SEVERITY(eFatal, "fatal", 4)
LOG_SEVERITY(ePanic, "panic", 5)

#undef LOG_SEVERITY
