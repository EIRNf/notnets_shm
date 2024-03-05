#!/bin/bash

compiler_cmd="$1"
# temp="${compiler_cmd%\'}"
# temp="${temp#\'}"=
# clear
rm -f bench
rm -f comparison


# compile and run c program
# clear
# perf stat -B -e cache-references,cache-misses,cycles,instructions,branches,faults,migrations
# perf c2c record -F 60000 -a
# perf stat -B -e faults,migrations,

$compiler_cmd ./bench.c -o bench
$compiler_cmd ./tcp_bench.c -o tcp_bench

# Compile run cpp program
# /usr/bin/clang++-17 -pthread  -Wall -Wextra -Werror -fcolor-diagnostics -fansi-escape-codes ./comparison.cpp -o ./comparison -g 
# /usr/bin/g++  -pthread -Wall -Wextra -Werror -g ./comparison.cpp -o ./comparison 

./bench
./tcp_bench
# ./comparison