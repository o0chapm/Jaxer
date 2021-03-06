--disable_abort_on_error

# ML: Should be set outside when memory
# set @@global.max_heap_table_size=4294967295;
# set @@session.max_heap_table_size=4294967295;

USE test;

--disable_warnings
DROP TABLE IF EXISTS t1, t2, t4, t10, t11;
--enable_warnings
eval CREATE TABLE t1  (f1 char(20), f2 char(25), f3 date, f4 int, f5 char(25), f6 int) ENGINE = $engine_type;
eval CREATE TABLE t2  (f1 char(20), f2 char(25), f3 date, f4 int, f5 char(25), f6 int) ENGINE = $engine_type;
eval CREATE TABLE t4  (f1 char(20), f2 char(25), f3 date, f4 int, f5 char(25), f6 int) ENGINE = $engine_type;
eval CREATE TABLE t10 (f1 char(20), f2 char(25), f3 date, f4 int, f5 char(25), f6 int) ENGINE = $engine_type;
eval CREATE TABLE t11 (f1 char(20), f2 char(25), f3 date, f4 int, f5 char(25), f6 int) ENGINE = $engine_type;
--replace_result $MYSQL_TEST_DIR MYSQL_TEST_DIR
eval LOAD DATA INFILE '$MYSQL_TEST_DIR/suite/funcs_1/data/t4.txt' INTO TABLE t1;
--replace_result $MYSQL_TEST_DIR MYSQL_TEST_DIR
eval LOAD DATA INFILE '$MYSQL_TEST_DIR/suite/funcs_1/data/t4.txt' INTO TABLE t2;
--replace_result $MYSQL_TEST_DIR MYSQL_TEST_DIR
eval LOAD DATA INFILE '$MYSQL_TEST_DIR/suite/funcs_1/data/t4.txt' INTO TABLE t4;
--replace_result $MYSQL_TEST_DIR MYSQL_TEST_DIR
eval LOAD DATA INFILE '$MYSQL_TEST_DIR/suite/funcs_1/data/t4.txt' INTO TABLE t10;
--replace_result $MYSQL_TEST_DIR MYSQL_TEST_DIR
eval LOAD DATA INFILE '$MYSQL_TEST_DIR/suite/funcs_1/data/t4.txt' INTO TABLE t11;


--disable_warnings
drop TABLE if exists t3;
--enable_warnings
eval CREATE TABLE t3 (f1 char(20), f2 char(20), f3 integer) ENGINE = $engine_type;
--replace_result $MYSQL_TEST_DIR MYSQL_TEST_DIR
eval LOAD DATA INFILE '$MYSQL_TEST_DIR/suite/funcs_1/data/t3.txt' INTO TABLE t3;

#---------------------------

--disable_warnings
drop database if exists test4;
--enable_warnings
CREATE database test4;
use test4;

eval CREATE TABLE t6 (f1 char(20), f2 char(25), f3 date, f4 int, f5 char(25), f6 int) ENGINE = $engine_type;
--replace_result $MYSQL_TEST_DIR MYSQL_TEST_DIR
eval LOAD DATA INFILE '$MYSQL_TEST_DIR/suite/funcs_1/data/t4.txt' INTO TABLE t6;
#---------------------------

use test;

--disable_warnings
drop TABLE if exists t7, t8;
--enable_warnings
eval CREATE TABLE t7 (f1 char(20), f2 char(25), f3 date, f4 int) ENGINE = $engine_type;
eval CREATE TABLE t8 (f1 char(20), f2 char(25), f3 date, f4 int) ENGINE = $engine_type;
--replace_result $MYSQL_TEST_DIR MYSQL_TEST_DIR
eval LOAD DATA INFILE '$MYSQL_TEST_DIR/suite/funcs_1/data/t7.txt' INTO TABLE t7;
--replace_result $MYSQL_TEST_DIR MYSQL_TEST_DIR
eval LOAD DATA INFILE '$MYSQL_TEST_DIR/suite/funcs_1/data/t7.txt' INTO TABLE t8;


--disable_warnings
drop TABLE if exists t9;
--enable_warnings
eval CREATE TABLE t9 (f1 int, f2 char(25), f3 int) ENGINE = $engine_type;
--replace_result $MYSQL_TEST_DIR MYSQL_TEST_DIR
eval LOAD DATA INFILE '$MYSQL_TEST_DIR/suite/funcs_1/data/t9.txt' INTO TABLE t9;

