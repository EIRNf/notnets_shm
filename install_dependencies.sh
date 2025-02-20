#!/bin/bash

# Function to handle errors
handle_error() {
    echo "Error occurred in script at line: $1"
    exit 1
}

# Trap errors
trap 'handle_error $LINENO' ERR

# Function to install a package
install_package() {
    sudo apt update
    local package=$1
    echo "Installing $package..."
    sudo apt-get install -y $package
}


build_clang(){
    wget https://apt.llvm.org/llvm.sh
    chmod u+x llvm.sh
    sudo ./llvm.sh 16
    alias clang++=clang++-16
    rm llvm.sh
}

# Function to build gtest
build_gtest() {
    echo "Building gtest..."
    sudo apt install -y cmake
    cd /usr/src/gtest || exit
    sudo cmake -DBUILD_SHARED_LIBS=ON .
    sudo make
    sudo cmake -DBUILD_SHARED_LIBS=OFF .
    sudo make
    # TODO: symbolic link instead of copy
    #  sudo cp lib/libgtest.so lib/libgtest_main.so lib/libgtest.a lib/libgtest_main.a /usr/local/lib

    sudo ln -s /usr/src/gtest/lib/libgtest.so /usr/local/lib/libgtest.so
    sudo ln -s /usr/src/gtest/lib/libgtest_main.so /usr/local/lib/libgtest_main.so
    sudo ln -s /usr/src/gtest/lib/libgtest.a /usr/local/lib/libgtest.a
    sudo ln -s /usr/src/gtest/lib/libgtest_main.a /usr/local/lib/libgtest_main.a
    sudo ln -s /usr/src/gtest/lib/libgtest.so.1.11.0 /usr/local/lib/libgtest.so.1.11.0
    sudo ln -s /usr/src/gtest/lib/libgtest_main.so.1.11.0 /usr/local/lib/libgtest_main.so.1.11.0

    # Run ldconfig
    sudo ldconfig
}

# Install clang
read -p "Do you want to install clang 16 or above? (y/n): " install_clang
if [[ $install_clang == "y" ]]; then
    build_clang
fi

# Install g++
read -p "Do you want to install g++ 12 or above? (y/n): " install_gpp
if [[ $install_gpp == "y" ]]; then
    install_package g++-12
fi

# Install boost
read -p "Do you want to install boost libraries? (y/n): " install_boost
if [[ $install_boost == "y" ]]; then
    install_package libboost-all-dev
fi

# Install gtest
read -p "Do you want to install gtest libraries? (y/n): " install_gtest
if [[ $install_gtest == "y" ]]; then
    install_package libgtest-dev
    build_gtest
fi

echo "All selected packages have been installed successfully."