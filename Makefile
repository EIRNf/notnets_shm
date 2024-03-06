.PHONY: all clean

all:
	make -C libnotnets all
	make -C bench_rpc all
	make -C test all

clean:
	make -C libnotnets clean
	make -C bench_rpc clean
	make -C test clean


