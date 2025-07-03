# OracleDB setup files when using daocc/orm

* create-test-user.sql - creates test user for unit tests

* tests assume certain parameters about oracledb
    * its reachable at `localhost:1521`
    * the `sys` account has a password `oracle`
    * there is a pluggable database named `FREEPDB1` attached

```sh
LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$(pwd)/subprojects/instantclient_23_8/instantclient_23_8" ./build/src/common/db/dbexec oracle localhost 1521 SYS oracle FREEPDB1 sysdba ./src/common/db/data/oracle/create-test-user.sql
t-user.sql
```
