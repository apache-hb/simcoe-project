#ifndef OPTION_TYPE
#   define OPTION_TYPE(ID, STR)
#endif

OPTION_TYPE(eUnknown, "unknown")
OPTION_TYPE(eBoolean, "bool")
OPTION_TYPE(eSigned, "signed")
OPTION_TYPE(eUnsigned, "unsigned")
OPTION_TYPE(eReal, "real")
OPTION_TYPE(eString, "string")
OPTION_TYPE(eSignedEnum, "enum")
OPTION_TYPE(eUnsignedEnum, "enum")

#undef OPTION_TYPE

#ifndef UPDATE_ERROR
#   define UPDATE_ERROR(ID, STR)
#endif

UPDATE_ERROR(eNone, "no error")
UPDATE_ERROR(eUnknownOption, "unknown option")
UPDATE_ERROR(eMissingValue, "missing argument")
UPDATE_ERROR(eInvalidValue, "invalid value")
UPDATE_ERROR(eOutOfRange, "value out of range")
UPDATE_ERROR(eSyntaxError, "syntax error")
UPDATE_ERROR(eReadOnly, "read-only option")

#undef UPDATE_ERROR
