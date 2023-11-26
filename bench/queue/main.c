// #include "../../src/queue.h"
//Currently commented out as it has not been updated
#include <stdio.h>

#include "bench.c"


/* Run client and server processes from same program distinguished by different arguments
Goals:
- Measure "echo" latency
- Measure throughput as inverse of latency as it is single threaded 
- Tune payload size
- Tune queue size
*/ 


int main()
{



    bench_run_all();
}

