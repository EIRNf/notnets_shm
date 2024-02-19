#!/bin/bash

compiler_cmd="$1"

# compiler_cmd="clang-17 -pthread -Wall -Wextra -Werror"

rm -f bench
rm -f comparison

# compile and run c program
# clear
# perf stat -B -e cache-references,cache-misses,cycles,instructions,branches,faults,migrations
# perf c2c record -F 60000 -a
# perf stat -B -e faults,migrations,

$compiler_cmd ./main.c -o bench

# Compile run cpp program
# /usr/bin/clang++-17 -pthread  -Wall -Wextra -Werror -fcolor-diagnostics -fansi-escape-codes ./comparison.cpp -o ./comparison -g 
/usr/bin/clang++-17  -pthread -Wall -Wextra -Werror -o2 ./comparison.cpp -o ./comparison 


#Attempt recording of CPU Utilization



./bench
./comparison