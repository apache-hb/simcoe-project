# OracleDB setup files when using daocc/orm

* setup.sql - all required store procedures
* test-setup.sql - all required setup to run orm unit tests
* blank-string-tests.sql - unit tests for IS_BLANK_STRING

* oracle tests assume a user called `TEST_USER` with the password `TEST_USER` is created on a locally running oracledb instance in a pluggable database called `FREEPDB1`.
