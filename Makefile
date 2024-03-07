.PHONY: all clean

all:
	make -C libnotnets all
	make -C bench_rpc all
	make -C bench_boost all
	make -C bench_tcp all
	make -C bench_queue all
	make -C bench_queue_boost all
	make -C test all

clean:
	make -C libnotnets clean
	make -C bench_rpc clean
	make -C bench_boost clean
	make -C bench_tcp clean
	make -C bench_queue clean
	make -C bench_queue_boost clean
	make -C test clean


