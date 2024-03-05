.PHONY: all clean bench

all:
	make -C libnotnets all
	make -C test all

clean:
	make -C libnotnets clean
	make -C test clean

bench:
	make bench -C bench 
