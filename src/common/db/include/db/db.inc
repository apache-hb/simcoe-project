#ifndef DB_TYPE
#   define DB_TYPE(id, name, enabled)
#endif

DB_TYPE(eSqlite3,    "sqlite3",    SMC_DB_HAS_SQLITE3)
DB_TYPE(ePostgreSQL, "PostgreSQL", SMC_DB_HAS_POSTGRES)
DB_TYPE(eMySQL,      "MySQL",      SMC_DB_HAS_MYSQL)
DB_TYPE(eOracleDB,   "OracleDB",   SMC_DB_HAS_ORCL)
DB_TYPE(eMSSQL,      "MS-SQL",     SMC_DB_HAS_MSSQL)
DB_TYPE(eDB2,        "IBM DB2",    SMC_DB_HAS_DB2)

#undef DB_TYPE

#ifndef DB_DATATYPE
#   define DB_DATATYPE(id, name)
#endif

DB_DATATYPE(eUnknown, "Unknown")
DB_DATATYPE(eInteger, "Integer")   // int32_t
DB_DATATYPE(eBoolean, "Boolean")   // bool

DB_DATATYPE(eString, "String")     // eChar/eVarChar

DB_DATATYPE(eChar, "Char")         // std::string
DB_DATATYPE(eVarChar, "VarChar")   // std::string
DB_DATATYPE(eDouble, "Double")     // double
DB_DATATYPE(eBlob, "Blob")         // Blob
DB_DATATYPE(eDateTime, "DateTime") // DateTime
DB_DATATYPE(eNull, "Null")         // ???
DB_DATATYPE(eRowId, "RowId")       // ???

#undef DB_DATATYPE

#ifndef DB_STATEMENT
#   define DB_STATEMENT(id, name)
#endif

DB_STATEMENT(eQuery, "Query")
DB_STATEMENT(eModify, "Modify")
DB_STATEMENT(eDefine, "Define")
DB_STATEMENT(eControl, "Control")

#undef DB_STATEMENT

#ifndef DB_JOURNAL
#   define DB_JOURNAL(id, name)
#endif

DB_JOURNAL(eDefault, "Default")

DB_JOURNAL(eDelete, "Delete")
DB_JOURNAL(eTruncate, "Truncate")
DB_JOURNAL(ePersist, "Persist")
DB_JOURNAL(eMemory, "Memory")
DB_JOURNAL(eWal, "Wal")
DB_JOURNAL(eOff, "Off")

#undef DB_JOURNAL

#ifndef DB_SYNC
#   define DB_SYNC(id, name)
#endif

DB_SYNC(eDefault, "Default")

DB_SYNC(eExtra, "Extra")
DB_SYNC(eFull, "Full")
DB_SYNC(eNormal, "Normal")
DB_SYNC(eOff, "Off")

#undef DB_SYNC

#ifndef DB_LOCKING
#   define DB_LOCKING(id, name)
#endif

DB_LOCKING(eDefault, "Default")

DB_LOCKING(eRelaxed, "Relaxed")
DB_LOCKING(eExclusive, "Exclusive")

#undef DB_LOCKING

#ifndef DB_ROLE
#   define DB_ROLE(id, name)
#endif

DB_ROLE(eDefault, "Default")

DB_ROLE(eSYSDBA, "SYSDBA")
DB_ROLE(eSYSOPER, "SYSOPER")
DB_ROLE(eSYSASM, "SYSASM")
DB_ROLE(eSYSBKP, "SYSBKP")
DB_ROLE(eSYSDGD, "SYSDGD")
DB_ROLE(eSYSKMT, "SYSKMT")
DB_ROLE(eSYSRAC, "SYSRAC")

#undef DB_ROLE
