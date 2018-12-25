# itskernel

## Building


### Prerequisites
The provided scripts assume a Debian-style Linux system with, among others, following packages installed:
* build-essential
* libgmp3-dev
* libmpfr-dev
* libmpc-dev
* texinfo
* bison
* flex

### Compiling build environment
The kernel requires a special GCC-based cross-compiler.

Navigate into the `compiler` sub directory and run the `buildcompiler.sh` script. This will download _binutils_ and _GCC_ and perform all necessary build steps. The generated binaries x86_64-elf-_name_ are stored in the `compiler/bin` sub directory, and can subsequently be used for compiling the kernel.

## License
The kernel itself is based on the [Arc Operating System](https://github.com/grahamedgecombe/arc) by Graham Edgecombe and licensed under the permissive [ISC license](https://www.isc.org/downloads/software-support-policy/isc-license/).

Other components:
* dlmalloc: The kernel uses Doug Lea's memory allocator, as released into the public domain using the [CC0](http://creativecommons.org/publicdomain/zero/1.0/) license. The license text can be found in LICENSE.dlmalloc.
* LWIP: Provides TCP/IP stack for network communication and uses a BSD license, which can be found in LICENSE.lwip.
