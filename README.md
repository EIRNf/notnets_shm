# notnets_shm
A library to handle message passing over shared memory. 


## Commands
Note: Cleanup implementation and makefile.

Currently all the src files are header files. As a result, the only executable programs that utilize the Notnets functions are found in the test and bench folders.
This needs to be changed. 


### Build Shared Library 

```
make
```
The default make command will build the object files and the library .so file

To rebuild the .so file call
```
make lib
```
### Run Tests
```
make test
```
Read outputs in tests/test_output.txt


### Run Benchmarks
```
make bench
```
Read outputs in bench/test_output.txt
