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

### Compiling the kernel and user-space UI application
The entire source tree is contained in the `code` sub directory. The directory has a central Makefile that allows to compile each of the components, by invoking `make `*`target`*. Supported targets:
* `kernel`: Builds the kernel itself.
* `lib`: Builds the user-space standard library.
* `net`: Builds the user-space network library (only for use by the UI process).
* `ui`: Builds the UI application.
* `all`: Builds all of the above.
* `clean`: Removes all compiler outputs, temporary files and binaries.

### Creating a disk image
After compiling all components these have to be combined into a disk image, to copy to an USB stick or load in a VM. This is done by executing `create_image.sh` from the `image` sub directory (sudo permission needed). To flash the image to an USB stick mounted at `/dev/sdb` invoke `sudo dd if=disk.img of=/dev/sdb` (be careful with this command).

### Implementing user-space applications
Other user-space applications are stored in the `code/apps/` sub directory; this directory already contains a "Hello World" program called `test` (which might be used as a template), and some apps written for research purposes.

The preferred way to load a user-space binary is using the network; otherwise, this can be achieved by adding the app to the OS image:
* In `image/grub.cfg`, add a line `module2 /bin/appname.elf appname` (the first argument specifies the path in the boot image, the second determines the application as shown in the OS file system)
* In `image/create_image.sh`, add a line `sudo cp ../code/apps/appname/appname.elf mnt/bin`
Then the application binary will be automatically mounted at `/apps/appname` during system startup.

## Debugging

### VMware
The directory `image/vm/` provides a pre-configured virtual machine file `itskernel.vmx` and a GDB input file `gdb.in`. The former configures VMware to open a debug port 1234, the latter attaches a GDB process to the running VM.

It is important to attach the debugger _after_ GRUB has completed; this can be achieved automatically by adding the line
```
monitor.debugOnStartGuest64 = "TRUE"
```
to the virtual machine configuration file, which causes the VM to automatically pause when hitting the kernel's `start` function.

The GDB input file automatically loads the kernel symbols and sets a break point at `init`; to debug user-space programs instead, set the respective executable as symbol file, and change the break point to e.g. `main`.

## License
The kernel itself is based on the [Arc Operating System](https://github.com/grahamedgecombe/arc) by Graham Edgecombe and licensed under the permissive [ISC license](https://www.isc.org/downloads/software-support-policy/isc-license/).

Other components:
* [dlmalloc](http://g.oswego.edu/dl/html/malloc.html): The kernel uses Doug Lea's memory allocator, as released into the public domain using the [CC0](http://creativecommons.org/publicdomain/zero/1.0/) license. The license text can be found in LICENSE.dlmalloc.
* [LWIP](https://savannah.nongnu.org/projects/lwip/): Provides TCP/IP stack for network communication. Licensed under a BSD license, which can be found in LICENSE.lwip.