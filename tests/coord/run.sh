#!/bin/bash

compiler_cmd="$1"
# temp="${compiler_cmd%\'}"
# temp="${temp#\'}"=

rm -f test
# clear

$compiler_cmd ./main.c -o test

./test