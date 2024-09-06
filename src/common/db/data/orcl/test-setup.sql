ALTER SESSION SET CONTAINER = FREEPDB1;

CREATE TABLESPACE TEST_USER_TBS
    DATAFILE 'D:/app/product/db23/oradata/FREE/FREEPDB1/test_user_tbs.dbf'
    SIZE 100M;

CREATE USER TEST_USER IDENTIFIED BY TEST_USER
    DEFAULT TABLESPACE TEST_USER_TBS
    QUOTA 100M ON TEST_USER_TBS;

GRANT CONNECT, RESOURCE TO TEST_USER;
GRANT CREATE SESSION TO TEST_USER;
GRANT UPDATE ANY TABLE TO TEST_USER;
GRANT DELETE ANY TABLE TO TEST_USER;
GRANT INSERT ANY TABLE TO TEST_USER;
GRANT SELECT ANY TABLE TO TEST_USER;
GRANT CREATE ANY TABLE TO TEST_USER;
