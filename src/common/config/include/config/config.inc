#ifndef OPTION_TYPE
#   define OPTION_TYPE(ID, STR)
#endif

OPTION_TYPE(eUnknown, "unknown")
OPTION_TYPE(eBoolean, "bool")
OPTION_TYPE(eInteger, "integer")
OPTION_TYPE(eReal, "real")
OPTION_TYPE(eString, "string")
OPTION_TYPE(eEnum, "enum")
OPTION_TYPE(eFlags, "flags")

#undef OPTION_TYPE
