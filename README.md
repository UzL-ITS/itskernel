# itskernel

## Table of contents
- [itskernel](#itskernel)
  * [Table of contents](#table-of-contents)
  * [Building](#building)
    + [Prerequisites](#prerequisites)
    + [Compiling build environment](#compiling-build-environment)
    + [Compiling the kernel and user-space programs](#compiling-the-kernel-and-user-space-programs)
    + [Creating a disk image](#creating-a-disk-image)
  * [License](#license)

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
* nasm

The kernel expects the following features on the target system:
* x86 architecture with 64-bit long mode
* PCI Express
* AVX, including XSAVE/XRSTOR instructions


### Compiling build environment
The kernel requires a special GCC-based cross-compiler.

Navigate into the `compiler` sub directory and run the `buildcompiler.sh` script. This will download _binutils_ and _GCC_ and perform all necessary build steps. The generated binaries `x86_64-elf-`*`name`* are stored in the `compiler/bin` sub directory, and can subsequently be used for compiling the kernel.

### Compiling the kernel and user-space programs
The entire source tree is contained in the `code` sub directory. The directory has a central Makefile that allows to compile each of the components, by invoking `make `*`target`*. Supported targets:
* `kernel`: Builds the kernel itself.
* `lib`: Builds the user-space standard library.
* `net`: Builds the user-space network library (only for use by the UI process).
* `ui`: Builds the UI application.
* `test`: Builds a small user-space test application.
* `all`: Builds all of the above.
* `clean`: Removes all compiler outputs, temporary files and binaries.

### Creating a disk image
After compiling all components these have to be combined into a disk image, to copy to an USB stick or load in a VM. This is done by executing `create_image.sh` from the `image` sub directory (sudo permission needed). To flash the image to an USB stick mounted at `/dev/sdb` invoke `sudo dd if=disk.img of=/dev/sdb` (be careful with this command).

## License
The kernel itself is based on the [Arc Operating System](https://github.com/grahamedgecombe/arc) by Graham Edgecombe and licensed under the permissive [ISC license](https://www.isc.org/downloads/software-support-policy/isc-license/).

Other components:
* [dlmalloc](http://g.oswego.edu/dl/html/malloc.html): The kernel uses Doug Lea's memory allocator, as released into the public domain using the [CC0](http://creativecommons.org/publicdomain/zero/1.0/) license. The license text can be found in LICENSE.dlmalloc.
* [LWIP](https://savannah.nongnu.org/projects/lwip/): Provides TCP/IP stack for network communication. Licensed under a BSD license, which can be found in LICENSE.lwip.