# OracleDB setup files when using daocc/orm

* create-test-user.sql - creates test user for unit tests

* tests assume certain parameters about oracledb
    * its reachable at `localhost:1521`
    * the `sys` account has a password `oracle`
    * there is a pluggable database named `FREEPDB1` attached
