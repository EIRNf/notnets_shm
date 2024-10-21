# notnets_shm
A library to handle message passing over shared memory. 

## Requirements
- clang
- boost lib
    - sudo apt-get install libboost-all-dev

## Commands
### Build Shared Library 

```
make
```
The default make command will build the object files, libnotnets .so file, benchmark binary, and test binary

```
make clean
```
Removes build artifacts

```
make install
```
Installs lib file on usr/local/lib
