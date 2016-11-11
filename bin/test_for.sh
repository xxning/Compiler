#!/bin/bash

# Generate IR
./bin/C1 ./test/test_for.c 2> 'test_for.ll'

# Generate Obj
llc-3.6 -filetype=obj test_for.ll -o test_for.o

# Link
clang test_for.o bin/mylib.a -o bin/Test_for

# run
./bin/Test_for
