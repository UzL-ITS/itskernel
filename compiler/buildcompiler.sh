#!/bin/bash

# Prepares the build environment:
# - Download and compile binutils
# - Download, patch and compile gcc toolchain for cross compilation

# Some needed packages:
# build-essential libgmp3-dev libmpfr-dev libmpc-dev texinfo bison flex

# References:
# - https://wiki.osdev.org/GCC_Cross-Compiler
# - https://wiki.osdev.org/Libgcc_without_red_zone

# Automatically terminate on errors
set -e

# Set some environment variables
export PATHPREFIX="$PWD"
export TARGET=x86_64-elf
export PATH="$PATHPREFIX/bin:$PATH"

# Check for some needed commands
if ! [ -x "$(command -v gcc)" ]; then
    echo "Error: gcc is not installed."
    exit 1
fi
if ! [ -x "$(command -v make)" ]; then
    echo "Error: make is not installed."
    exit 1
fi
if ! [ -x "$(command -v patch)" ]; then
    echo "Error: patch is not installed."
    exit 1
fi

# Check whether output directories do already exist
forceRecompile=false
if [ -d bin ]; then
    
    read -p "Detected existing output directories. Remove old files? This requires recompiling all components. (y/n) " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        rm -rf bin
        rm -rf share
        rm -rf $TARGET
        forceRecompile=true
    fi
fi

# Check whether compiled binutils directory does already exist
echo "Checking for binutils..."
compileBinutils=true
if [ -d binutils-2.30 ] && [ "$forceRecompile" = false ]; then
    
    read -p "Detected existing binutils source directory. Recompile? (y/n) " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        rm -rf binutils-2.30
    else
        compileBinutils=false
    fi
fi

# Compile binutils
if [ "$compileBinutils" = true ]; then

    echo "Checking for binutils source archive..."
    if [ ! -f binutils-2.30.tar.gz ]; then
        echo "Source archive not found, downloading..."
        wget http://ftpmirror.gnu.org/binutils/binutils-2.30.tar.gz
    fi
    
    echo "Extracting binutils source archive..."
    tar -xzf binutils-2.30.tar.gz
    
    echo "Creating binutils build directory..."
    if [ -d build-binutils ]; then
        rm -rf build-binutils
    fi
    mkdir build-binutils
    cd build-binutils
    
    echo "Configuring binutils..."
    ../binutils-2.30/configure --target=$TARGET --prefix="$PATHPREFIX" --with-sysroot --disable-nls --disable-werror
    
    echo "Building binutils..."
    make
    
    echo "Installing binutils..."
    make install
    
    echo "Building binutils completed."
	cd ..
fi

# Check whether compiled gcc directory does already exist
echo "Checking for gcc..."
compileGcc=true
if [ -d gcc-8.1.0 ] && [ "$forceRecompile" = false ]; then
    
    read -p "Detected existing gcc source directory. Recompile? (y/n) " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        rm -rf gcc-8.1.0
    else
        compileGcc=false
    fi
fi

# Compile gcc
if [ "$compileGcc" = true ]; then

    echo "Checking for gcc source archive..."
    if [ ! -f gcc-8.1.0.tar.gz ]; then
        echo "Source archive not found, downloading..."
        wget http://ftpmirror.gnu.org/gcc/gcc-8.1.0/gcc-8.1.0.tar.gz
    fi
    
    echo "Extracting gcc source archive..."
    tar -xzf gcc-8.1.0.tar.gz
    
    echo "Patching libgcc for -mno-red-zone support"
    cp t-x86_64-elf gcc-8.1.0/gcc/config/i386/
    patch gcc-8.1.0/gcc/config.gcc < config.gcc.patch
    
    echo "Creating gcc build directory..."
    if [ -d build-gcc ]; then
        rm -rf build-gcc
    fi
    mkdir build-gcc
    cd build-gcc
    
    echo "Configuring gcc..."
    ../gcc-8.1.0/configure --target=$TARGET --prefix="$PATHPREFIX" --disable-nls --enable-languages=c,c++ --without-headers
    
    echo "Building gcc..."
    make all-gcc
    make all-target-libgcc
    
    echo "Installing gcc..."
    make install-gcc
    make install-target-libgcc
    
    echo "Building gcc completed."
	cd ..
fi

# Tests
echo "Testing gcc..."
$TARGET-gcc --version

# Done
echo "Build environment is ready."