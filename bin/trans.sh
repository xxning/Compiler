#!/bin/sh

bin/C1 test/test_for.c 	2> 'test_for.ll'
bin/C1 test/test_logic.c    2> 'test_logic.ll'
bin/C1 test/test_BC.c 	2> 'test_BC.ll'
bin/C1 test/test_fun.c 	2> 'test_fun.ll'
bin/C1 test/test_Err.c -d test_Err.dot
dot -Tpng test_Err.dot -o test_Err.png
bin/C1 test/test_complex1.c 2> 'test_complex1.ll'
bin/C1 test/test_complex2.c 2> 'test_complex2.ll'







