#!/bin/bash

compiler_cmd="$1"

rm -f bench
# clear
# perf stat -B -e cache-references,cache-misses,cycles,instructions,branches,faults,migrations
$compiler_cmd ./main.c -o bench

./bench