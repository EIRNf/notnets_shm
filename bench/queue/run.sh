#!/bin/bash

compiler_cmd="$1"

# compiler_cmd="clang-17 -pthread -Wall -Wextra -Werror"

rm -f bench
rm -f comparison

# clear
# perf stat -B -e cache-references,cache-misses,cycles,instructions,branches,faults,migrations
$compiler_cmd   ./main.c -o bench

# Compile run cpp program

# /usr/bin/clang++-17 -pthread  -Wall -Wextra -Werror -fcolor-diagnostics -fansi-escape-codes ./comparison.cpp -o ./comparison -g 
/usr/bin/g++  -pthread -Wall -Wextra -Werror -g ./comparison.cpp -o ./comparison 


./bench
./comparison