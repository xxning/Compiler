#!/bin/bash

# Generate IR
./bin/C1 ./test/mylib_test.c 2> 'mylib_test.ll'

# Generate Obj
llc-3.6 -filetype=obj mylib_test.ll -o mylib_test.o

# Link
clang mylib_test.o bin/mylib.a -o bin/Test

# run
./bin/Test
