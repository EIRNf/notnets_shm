#!/bin/bash

compiler_cmd="$1"

rm -f test
# clear

$compiler_cmd ./main.c -o test

./test
