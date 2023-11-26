#!/bin/bash

compiler_cmd="$1"
# temp="${compiler_cmd%\'}"
# temp="${temp#\'}"=

rm -f bench
# clear

$compiler_cmd ./main.c -o bench

./bench