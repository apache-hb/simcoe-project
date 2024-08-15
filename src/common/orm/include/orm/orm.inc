#ifndef DB_TYPE
#   define DB_TYPE(id, str, enabled)
#endif

DB_TYPE(eSqlite3, "sqlite3", true)
DB_TYPE(ePostgreSQL, "PostgreSQL", HAS_POSTGRES)
DB_TYPE(eMySQL, "MySQL", HAS_MYSQL)
DB_TYPE(eOracleDB, "OracleDB", HAS_ORCL)
DB_TYPE(eMSSQL, "MSSQL", HAS_MSSQL)
DB_TYPE(eDB2, "DB2", HAS_DB2)
DB_TYPE(eODBC, "ODBC", HAS_ODBC)

#undef DB_TYPE
