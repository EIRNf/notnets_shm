# notnets_shm
A library to handle message passing over shared memory. 

## Requirements
- clang 16 or above
- g++ 12 or above
- boost lib
    - sudo apt-get install libboost-all-dev
- gtest lib
    - sudo apt-get install libgtest-dev
    - cd /usr/src/gtest
    - sudo cmake -DBUILD_SHARED_LIBS=ON .
    - sudo make
    - sudo cmake -DBUILD_SHARED_LIBS=OFF .
    - sudo make
    - sudo cp libgtest.so (libgtest_main.so/libgtest.a/libgtest_main.a) /usr/local/lib

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
