#!/bin/bash

compiler_cmd="$1"

rm -f bench
# clear
# perf stat -B -e cache-references,cache-misses,cycles,instructions,branches,faults,migrations
$compiler_cmd ./main.c -o bench

# Compile run cpp program

# /usr/bin/clang -fcolor-diagnostics -fansi-escape-codes -g /Users/estebanramos/projects/notnets_shm/bench/queue/comparison.cpp -o /Users/estebanramos/projects/notnets_shm/bench/queue/comparison -I/opt/homebrew/include -L/opt/homebrew/lib




./bench
# ./comparison